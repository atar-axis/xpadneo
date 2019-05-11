from evdev import ecodes, InputDevice, ff, util
import time

dev = None

# Find first EV_FF capable event device (that we have permissions to use).
for name in util.list_devices():
    dev = InputDevice(name)
    if ecodes.EV_FF in dev.capabilities():
        break

if dev is None:

    print("Sorry, no FF capable device found")

else:
    print("found " + dev.name + " at " + dev.path)
    print("Preparing FF effect...")

    rumble = ff.Rumble(strong_magnitude=0xc000, weak_magnitude=0xc000)
    effect_type = ff.EffectType(ff_rumble_effect=rumble)
    duration_ms = 1000

    effect = ff.Effect(
        ecodes.FF_RUMBLE, # type
        -1, # id (set by ioctl)
        0,  # direction
        ff.Trigger(0, 0), # no triggers
        ff.Replay(duration_ms, 0), # length and delay
        ff.EffectType(ff_rumble_effect=rumble)
    )

    print("Uploading FF effect...")

    effect_id = dev.upload_effect(effect)


    print("Playing FF effect...")

    repeat_count = 1


    dev.write(ecodes.EV_FF, effect_id, repeat_count)
    time.sleep(1)

    dev.erase_effect(effect_id)
