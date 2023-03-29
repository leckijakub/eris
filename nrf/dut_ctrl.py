#!/usr/bin/env python3

import serial
import argparse


ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)


def reset(args):
    ser.write(b"reset\r")


def jam(args):
    ser.write(f"jam {args.power_lvl}\r".encode())

def tx(args):
    ser.write(b"tx\r")

def rx(args):
    ser.write(b"rx\r")

def idle(args):
    ser.write(b"idle\r")

def write(args):
    ser.write(f"{args.command}\r".encode())


parser = argparse.ArgumentParser()
parser.add_argument("device", help="Device to communicate")
subparsers = parser.add_subparsers(help="actions")
reset_parser = subparsers.add_parser("reset", help="Reset the board")
reset_parser.set_defaults(func=reset)
jam_parser = subparsers.add_parser("jam", help="Set board to jam")
jam_parser.add_argument("power_lvl", type=int, choices=range(0,6), help="power level of jamming")
jam_parser.set_defaults(func=jam)
idle_parser = subparsers.add_parser("idle", help="Set board to idle")
idle_parser.set_defaults(func=idle)
tx_parser = subparsers.add_parser("tx", help="Set board to tx")
tx_parser.set_defaults(func=tx)

rx_parser = subparsers.add_parser("rx", help="Set board to rx")
rx_parser.set_defaults(func=rx)

write_parser = subparsers.add_parser("write", help="Write custom command")
write_parser.add_argument("command", default="", help="command to write")
write_parser.set_defaults(func=write)

args = parser.parse_args()

print(ser.readall().decode())
try:
    args.func(args)
except AttributeError:
    parser.error("error while executing action")
try:
    print(ser.readall().decode())
except serial.SerialException:
    pass

ser.close()
