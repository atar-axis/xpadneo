#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
example class to use the controller with asyncio
python>=3.6 is necessary
script is tested on a raspberry pi 3
"""
import asyncio
from evdev import InputDevice, ff, ecodes

class gamepad():
    def __init__(self, file = '/dev/input/event0'):
        #self.event_value = 0
        self.power_on = True
        self.device_file = InputDevice(file)
        self.joystick_left_y = 0 # values are mapped to [-1 ... 1]
        self.joystick_left_x = 0 # values are mapped to [-1 ... 1]
        self.joystick_right_x = 0 # values are mapped to [-1 ... 1]
        self.trigger_right = 0 # values are mapped to [0 ... 1]
        self.trigger_left = 0 # values are mapped to [0 ... 1]
        self.button_x = False
        self.button_y = False
        self.button_b = False
        self.rumble_effect = 0
        self.effect1_id = 0 # light rumble, played continuously
        self.effect2_id = 0 # strong rumble, played once
        self.load_effects()

    def load_effects(self):
        #effect 1, light rumble
        rumble = ff.Rumble(strong_magnitude=0x0000, weak_magnitude=0x500)
        duration_ms = 300
        effect = ff.Effect(ecodes.FF_RUMBLE, -1, 0, ff.Trigger(0, 0), ff.Replay(duration_ms, 0), ff.EffectType(ff_rumble_effect=rumble))
        self.effect1_id = self.device_file.upload_effect(effect)
        # effect 2, strong rumble
        rumble = ff.Rumble(strong_magnitude=0xc000, weak_magnitude=0x0000)
        duration_ms = 200
        effect = ff.Effect(ecodes.FF_RUMBLE, -1, 0, ff.Trigger(0, 0), ff.Replay(duration_ms, 0), ff.EffectType(ff_rumble_effect=rumble))
        self.effect2_id = self.device_file.upload_effect(effect)


    async def read_gamepad_input(self): # asyncronus read-out of events
        max_abs_joystick_left_x = 0xFFFF/2
        uncertainty_joystick_left_x = 2500
        max_abs_joystick_left_y = 0xFFFF/2
        uncertainty_joystick_left_y = 2500
        max_abs_joystick_right_x = 0xFFFF/2
        uncertainty_joystick_right_x = 2000
        max_trigger = 1023

        async for event in self.device_file.async_read_loop():
                if not(self.power_on): #stop reading device when power_on = false
                    break
                if event.type == 3: # type is analog trigger or joystick
                    if event.code == 1: # left joystick y-axis
                        if -event.value > uncertainty_joystick_left_y:
                            self.joystick_left_y = (-event.value - uncertainty_joystick_left_y) / (max_abs_joystick_left_y - uncertainty_joystick_left_y + 1)
                        elif -event.value < -uncertainty_joystick_left_y:
                            self.joystick_left_y = (-event.value + uncertainty_joystick_left_y) / (max_abs_joystick_left_y - uncertainty_joystick_left_y + 1)
                        else:
                            self.joystick_left_y = 0
                    elif event.code == 0: # left joystick x-axis
                        if event.value > uncertainty_joystick_left_x:
                            self.joystick_left_x = (event.value - uncertainty_joystick_left_x) / (max_abs_joystick_left_x - uncertainty_joystick_left_x + 1)
                        elif event.value < -uncertainty_joystick_left_x:
                            self.joystick_left_x = (event.value + uncertainty_joystick_left_x) / (max_abs_joystick_left_x - uncertainty_joystick_left_x + 1)
                        else:
                            self.joystick_left_x = 0
                    elif event.code == 3: # right joystick x-axis
                        if event.value > uncertainty_joystick_right_x:
                            self.joystick_right_x = (event.value - uncertainty_joystick_right_x) / (max_abs_joystick_right_x - uncertainty_joystick_right_x + 1)
                        elif event.value < -uncertainty_joystick_right_x:
                            self.joystick_right_x = (event.value + uncertainty_joystick_right_x) / (max_abs_joystick_right_x - uncertainty_joystick_right_x + 1)
                        else:
                            self.joystick_right_x = 0
                    elif event.code == 5: # right trigger
                        self.trigger_right = event.value / max_trigger
                    elif event.code == 2: # left trigger
                        self.trigger_left = event.value / max_trigger
                if (event.type == 1): # type is button
                    if event.code == 307: # button "X" pressed ?
                        self.button_x = True
                    if event.code == 308: # button "Y" pressed ?
                        self.button_y = True
                    if event.code == 305: # button "B" pressed ?
                        self.button_b = True

    async def rumble(self): # asyncronus control of force feed back effects
        repeat_count = 1
        while self.power_on:
            if self.rumble_effect == 1:
                self.device_file.write(ecodes.EV_FF, self.effect1_id, repeat_count)
            elif self.rumble_effect == 2:
                self.device_file.write(ecodes.EV_FF, self.effect2_id, repeat_count)
                self.rumble_effect = 0 # turn of effect in order to play effect2 only once
            await asyncio.sleep(0.2)

    def erase_rumble(self):
        self.device_file.erase_effect(self.effect1_id)

if __name__ == "__main__":

    async def main():
        print("press x to stop, Y to rumble light, B to rumble once, trigger and joystick right to see analog value")
        while True:
            print(" trigger_right = ", round(remote_control.trigger_right,2), "  joystick_right_x = ", round(remote_control.joystick_right_x,2),end="\r")
            if remote_control.button_y: # turn on light rumble effect
                remote_control.button_y = False
                remote_control.rumble_effect = 1
            if remote_control.button_b: # play once strong rumble effect
                remote_control.button_b = False
                remote_control.rumble_effect = 2
            if remote_control.button_x: # stop the script
                remote_control.power_on = False
                remote_control.erase_rumble()
                break
            await asyncio.sleep(0)

    remote_control = gamepad(file = '/dev/input/event0')
    futures = [remote_control.read_gamepad_input(), remote_control.rumble(), main()]
    loop = asyncio.get_event_loop()
    loop.run_until_complete(asyncio.wait(futures))
    loop.close()
    print(" ")
