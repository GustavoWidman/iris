#!/usr/bin/env python3
"""Bounded serial capture/test; no daemon/autostart, no credential input.

An optional owner-only command queue can feed a *single already-open* serial
session. It is intentionally limited to the temporary RGB-test commands, so
it cannot factory-reset, commission, or alter networking.
"""

import argparse
import os
import math
import pathlib
import re
import stat
import time

QUEUE_COMMAND = re.compile(
    r"(?:rgb (?:0|[1-9][0-9]{0,1}|1[0-9]{2}|2[0-4][0-9]|25[0-5]) (?:0|[1-9][0-9]{0,1}|1[0-9]{2}|2[0-4][0-9]|25[0-5]) (?:0|[1-9][0-9]{0,1}|1[0-9]{2}|2[0-4][0-9]|25[0-5])|resume|status)"
)


def owner_only_regular(path: pathlib.Path) -> None:
    """Reject symlinks, foreign-owned, group-readable, or world-readable files."""
    info = os.lstat(path)
    if not stat.S_ISREG(info.st_mode):
        raise ValueError(f"queue must be a regular file: {path}")
    if info.st_uid != os.getuid() or info.st_mode & 0o077:
        raise ValueError(f"queue must be owned by this user and mode 0600: {path}")


def read_queue(path: pathlib.Path, offset: int):
    """Return complete, allowed newline-delimited commands and a safe offset."""
    owner_only_regular(path)
    size = path.stat().st_size
    if size < offset:  # A deliberate truncate starts a new queue generation.
        offset = 0
    with path.open("rb") as queue:
        queue.seek(offset)
        data = queue.read()
    complete = data.rsplit(b"\n", 1)
    if len(complete) == 1:
        return [], offset
    payload, remainder = complete
    next_offset = offset + len(payload) + 1
    commands = []
    for raw in payload.split(b"\n"):
        try:
            command = raw.decode("ascii")
        except UnicodeDecodeError:
            command = ""
        if QUEUE_COMMAND.fullmatch(command):
            commands.append(command)
        else:
            commands.append(None)
    return commands, next_offset


def open_private_output(path: pathlib.Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    flags = os.O_CREAT | os.O_WRONLY | os.O_TRUNC
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    fd = os.open(path, flags, 0o600)
    os.fchmod(fd, 0o600)
    return os.fdopen(fd, "wb")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="explicit serial device; no auto-discovery")
    parser.add_argument("--seconds", type=float, default=60)
    parser.add_argument("--output", default="evidence/serial.log")
    parser.add_argument("--command", action="append", default=[], help="SECONDS:COMMAND")
    parser.add_argument(
        "--command-queue",
        type=pathlib.Path,
        help="existing mode-0600 local queue; allowed: rgb R G B, resume, status",
    )
    args = parser.parse_args()
    if not math.isfinite(args.seconds) or not 0 < args.seconds <= 3600:
        parser.error("--seconds must be finite and between 0 and 3600")
    commands = []
    for item in args.command:
        delay, command = item.split(":", 1)
        delay = float(delay)
        if not math.isfinite(delay) or not 0 <= delay < args.seconds:
            parser.error("command delay must be finite and within the session")
        commands.append((delay, command))
    commands.sort()
    queue_offset = 0
    if args.command_queue:
        owner_only_regular(args.command_queue)
    destination = pathlib.Path(args.output)
    import serial  # Only after argument validation, never for --help.

    # Configure modem lines before opening; opening with defaults can reset ESP32.
    connection = serial.Serial(port=None, baudrate=115200, timeout=0.25, exclusive=True)
    connection.dtr = False
    connection.rts = False
    connection.port = args.port
    with open_private_output(destination) as log, connection:
        start = time.monotonic()
        while time.monotonic() - start < args.seconds:
            while commands and time.monotonic() - start >= commands[0][0]:
                _, command = commands.pop(0)
                connection.write((command + "\n").encode())
                log.write(("\nCOMMAND " + command + "\n").encode())
            if args.command_queue:
                queued, queue_offset = read_queue(args.command_queue, queue_offset)
                for command in queued:
                    if command is None:
                        log.write(b"\nQUEUE_REJECTED\n")
                    else:
                        connection.write((command + "\n").encode("ascii"))
                        log.write(("\nCOMMAND_QUEUE " + command + "\n").encode("ascii"))
                if queued:
                    log.flush()
            data = connection.read(connection.in_waiting or 1)
            if data:
                log.write(data)
                log.flush()
    text = destination.read_text(errors="replace")
    clean = re.sub(r"\x1b\[[0-9;]*m", "", text)
    for line in clean.splitlines():
        if (
            "BJ_" in line
            or "BJ_LED" in line
            or "GATT uuid" in line
            or "panic" in line
            or "assert" in line
        ) and "password" not in line.lower():
            print(line)
    print("Captured", destination)


if __name__ == "__main__":
    main()
