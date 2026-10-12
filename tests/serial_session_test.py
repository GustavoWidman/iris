#!/usr/bin/env python3
import importlib.util
import os
import pathlib
import tempfile

spec = importlib.util.spec_from_file_location("serial_session", pathlib.Path("tools/serial_session.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as temporary:
    root = pathlib.Path(temporary)
    queue = root / "queue"
    queue.write_text("rgb 1 2 3\nresume\nstatus\nrgb 256 0 0\nreboot\n")
    queue.chmod(0o600)
    commands, offset = module.read_queue(queue, 0)
    assert commands == ["rgb 1 2 3", "resume", "status", None, None]
    assert offset == queue.stat().st_size
    # A partial append is retained until it is newline-delimited.
    with queue.open("a") as handle:
        handle.write("rgb 9 8")
    commands, unchanged = module.read_queue(queue, offset)
    assert commands == [] and unchanged == offset
    with queue.open("a") as handle:
        handle.write(" 7\n")
    commands, offset = module.read_queue(queue, offset)
    assert commands == ["rgb 9 8 7"]
    queue.chmod(0o640)
    try:
        module.owner_only_regular(queue)
        raise AssertionError("group-readable queue accepted")
    except ValueError:
        pass
