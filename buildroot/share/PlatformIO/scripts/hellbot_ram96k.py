#
# Hellbot Magna SE (Wanhao 0327001 / AT32F403ARGT7):
# this unit only exposes 96KB of SRAM (option-byte EOPB0 set to 96KB at
# the factory; verified via SWD: accesses >= 0x20018000 fail).
# The stock linker script declares 224KB, so the initial SP lands in
# non-existent RAM and the firmware HardFaults immediately (see
# HANDOFF-SE.md section 4).  Patch the linker script before every build
# so the fix survives a re-install of the platform packages in /tmp.
#
Import("env")

import os

LD = os.path.join(env["PROJECT_PACKAGES_DIR"],
                  "framework-arduino-at32f4", "variants",
                  "marlin_at32f403", "linker_scripts",
                  "AT32F403AxG_FLASH.ld")

ESTACK_PATCHED = "_estack = 0x20018000;"
RAM_PATCHED    = "LENGTH = 96K"
RAM_ORIG       = "LENGTH = 224K"

def patch_ram96k(*args, **kwargs):
    if not os.path.exists(LD):
        print("hellbot_ram96k: WARNING linker script not found: %s" % LD)
        return
    with open(LD) as f:
        text = f.read()

    if ESTACK_PATCHED in text and RAM_PATCHED in text:
        print("hellbot_ram96k: linker script already limited to 96KB SRAM")
        return

    bak = LD + ".orig-224k"
    if not os.path.exists(bak):
        with open(bak, "w") as f:
            f.write(text)

    lines = text.splitlines(keepends=True)
    for i, line in enumerate(lines):
        if "_estack" in line and ESTACK_PATCHED not in line:
            lines[i] = ESTACK_PATCHED + " /* Hellbot SE: 96KB SRAM visible */\n"
        elif RAM_ORIG in line and "RAM" in line:
            lines[i] = line.replace(RAM_ORIG, RAM_PATCHED)

    with open(LD, "w") as f:
        f.writelines(lines)
    print("hellbot_ram96k: patched linker -> 96KB SRAM (_estack 0x20018000); backup at %s" % bak)

patch_ram96k()
