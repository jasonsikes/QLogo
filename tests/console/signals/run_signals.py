#!/usr/bin/env python3
"""Drive qlogo on a PTY, send Unix signals, and check the transcript.

The piped console suite never generates tty signals. These cases start qlogo
as a session leader, send SIGINT/SIGQUIT/SIGTSTP from the parent, and check
that the Logo actions match UCBLogo (THROW "TOPLEVEL, THROW "SYSTEM, PAUSE).
"""

from __future__ import annotations

import os
import pty
import select
import signal
import sys
import termios
import time


CATCH_TAG_MSG = "Can't find catch tag"


class Timeout(Exception):
    pass


class Failed(Exception):
    def __init__(self, name: str, message: str, transcript: str):
        super().__init__(message)
        self.name = name
        self.message = message
        self.transcript = transcript


class Session:
    def __init__(self, logo_path: str, extra_args: list[str] | None = None):
        args = [logo_path, "--no-tui"]
        if extra_args:
            args.extend(extra_args)
        self.pid, self.master = pty.fork()
        if self.pid == 0:
            os.execv(logo_path, args)
            os._exit(127)
        attrs = termios.tcgetattr(self.master)
        attrs[3] &= ~termios.ECHO
        termios.tcsetattr(self.master, termios.TCSANOW, attrs)
        self.buf = ""

    def _read_once(self, timeout: float) -> bool:
        ready, _, _ = select.select([self.master], [], [], timeout)
        if not ready:
            return False
        try:
            chunk = os.read(self.master, 4096)
        except OSError:
            return False
        if not chunk:
            return False
        text = chunk.decode("utf-8", errors="replace").replace("\r\n", "\n").replace("\r", "")
        self.buf += text
        return True

    def mark(self) -> int:
        return len(self.buf)

    def wait_for(self, needle: str, timeout: float = 12.0, after: int = 0) -> None:
        deadline = time.monotonic() + timeout
        while needle not in self.buf[after:]:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise Timeout(f"timed out waiting for {needle!r}\ntranscript:\n{self.buf}")
            self._read_once(min(remaining, 0.4))

    def wait_quiet(self, quiet: float = 0.25, timeout: float = 8.0) -> None:
        deadline = time.monotonic() + timeout
        last = time.monotonic()
        while time.monotonic() < deadline:
            if self._read_once(0.05):
                last = time.monotonic()
                continue
            if time.monotonic() - last >= quiet:
                return
        raise Timeout(f"did not go quiet\ntranscript:\n{self.buf}")

    def write(self, text: str) -> None:
        os.write(self.master, text.encode("utf-8"))

    def send(self, sig: int) -> None:
        os.kill(self.pid, sig)

    def wait_exit(self, timeout: float = 8.0) -> int:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            self._read_once(0.05)
            try:
                pid, status = os.waitpid(self.pid, os.WNOHANG)
            except ChildProcessError:
                return 0
            if pid == self.pid:
                try:
                    os.close(self.master)
                except OSError:
                    pass
                if os.WIFEXITED(status):
                    return os.WEXITSTATUS(status)
                if os.WIFSIGNALED(status):
                    return -os.WTERMSIG(status)
                return status
        try:
            os.kill(self.pid, signal.SIGKILL)
        except OSError:
            pass
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass
        try:
            os.close(self.master)
        except OSError:
            pass
        return -9

    def close(self, timeout: float = 5.0) -> int:
        return self.wait_exit(timeout)

    def bye(self) -> int:
        try:
            self.write("bye\n")
        except OSError:
            pass
        return self.wait_exit()


def require(session: Session, cond: bool, name: str, message: str) -> None:
    if not cond:
        raise Failed(name, message, session.buf)


def wait_prompt(session: Session, prompt: str = "? ", after: int = 0) -> None:
    session.wait_for(prompt, after=after)


def test_sigint_forever(logo_path: str) -> None:
    name = "sigint_forever"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write("forever [print 1]\n")
        s.wait_for("1\n", after=mark)
        time.sleep(0.15)
        s.send(signal.SIGINT)
        wait_prompt(s, after=mark)
        require(s, CATCH_TAG_MSG not in s.buf, name, "SIGINT printed an error")
        mark = s.mark()
        s.write("print [ok]\n")
        s.wait_for("ok", after=mark)
        s.bye()
    except Exception:
        s.close()
        raise


def test_catch_toplevel(logo_path: str) -> None:
    name = "catch_toplevel"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write('catch "toplevel [forever [print 1]]\nprint [after]\n')
        s.wait_for("1\n", after=mark)
        time.sleep(0.15)
        s.send(signal.SIGINT)
        s.wait_for("after", after=mark)
        require(s, CATCH_TAG_MSG not in s.buf, name, "caught SIGINT still printed an error")
        s.bye()
    except Exception:
        s.close()
        raise


def test_sigquit_exits(logo_path: str) -> None:
    name = "sigquit_exits"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write("to foo\nforever [print 1]\nend\nfoo\n")
        s.wait_for("1\n", after=mark)
        time.sleep(0.15)
        s.send(signal.SIGQUIT)
        status = s.wait_exit()
        require(s, CATCH_TAG_MSG not in s.buf, name, "SIGQUIT printed catch-tag error")
        require(s, status >= 0, name, f"SIGQUIT killed the process with signal {-status}")
    except Exception:
        s.close()
        raise


def test_catch_system(logo_path: str) -> None:
    name = "catch_system"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write('catch "system [forever [print 1]]\nprint [lived]\n')
        s.wait_for("1\n", after=mark)
        time.sleep(0.15)
        s.send(signal.SIGQUIT)
        s.wait_for("lived", after=mark)
        require(s, CATCH_TAG_MSG not in s.buf, name, "CATCH \"SYSTEM printed an error")
        s.bye()
    except Exception:
        s.close()
        raise


def test_sigtstp_procedure(logo_path: str) -> None:
    name = "sigtstp_procedure"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write("to foo\nlocalmake \"x 99\nforever [wait 60]\nend\nfoo\n")
        s.wait_for("foo defined", after=mark)
        time.sleep(0.2)
        s.send(signal.SIGTSTP)
        s.wait_for("Pausing...", after=mark)
        s.wait_for("foo? ", after=mark)
        mark = s.mark()
        s.write("print :x\n")
        s.wait_for("99", after=mark)
        s.write("continue\n")
        time.sleep(0.2)
        s.send(signal.SIGINT)
        wait_prompt(s, after=mark)
        s.bye()
    except Exception:
        s.close()
        raise


def test_continue_value(logo_path: str) -> None:
    name = "continue_value"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write("print pause\n")
        s.wait_for("Pausing...", after=mark)
        wait_prompt(s, after=mark)
        mark = s.mark()
        s.write("continue 7\n")
        s.wait_for("7", after=mark)
        s.bye()
    except Exception:
        s.close()
        raise


def test_already_pausing(logo_path: str) -> None:
    name = "already_pausing"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write("pause\n")
        s.wait_for("Pausing...", after=mark)
        wait_prompt(s, after=mark)
        s.send(signal.SIGTSTP)
        s.wait_for("Already Pausing", after=mark)
        mark = s.mark()
        s.write("continue\n")
        wait_prompt(s, after=mark)
        s.bye()
    except Exception:
        s.close()
        raise


def test_sigint_during_pause(logo_path: str) -> None:
    name = "sigint_during_pause"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write("to foo\npause\nprint [should-not-see]\nend\nfoo\n")
        s.wait_for("Pausing...", after=mark)
        s.wait_for("foo? ", after=mark)
        s.send(signal.SIGINT)
        wait_prompt(s, after=s.mark())
        require(s, "should-not-see" not in s.buf, name, "SIGINT during pause resumed the procedure")
        mark = s.mark()
        s.write("print [back]\n")
        s.wait_for("back", after=mark)
        s.bye()
    except Exception:
        s.close()
        raise


def test_sigint_at_prompt(logo_path: str) -> None:
    name = "sigint_at_prompt"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.send(signal.SIGINT)
        time.sleep(0.3)
        s.wait_quiet()
        require(s, CATCH_TAG_MSG not in s.buf, name, "SIGINT at prompt printed an error")
        s.write("print [stillhere]\n")
        s.wait_for("stillhere", after=mark)
        s.bye()
    except Exception:
        s.close()
        raise


def test_sigint_during_to(logo_path: str) -> None:
    name = "sigint_during_to"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write("to foo\n")
        s.wait_for("> ", after=mark)
        s.send(signal.SIGINT)
        wait_prompt(s, after=s.mark())
        require(s, "foo defined" not in s.buf, name, "SIGINT during TO defined a partial procedure")
        mark = s.mark()
        s.write("print definedp \"foo\n")
        s.wait_for("false", after=mark)
        s.bye()
    except Exception:
        s.close()
        raise


def test_sigint_during_wait(logo_path: str) -> None:
    name = "sigint_during_wait"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        mark = s.mark()
        s.write('print "start\nwait 10000\nprint "done\n')
        s.wait_for("start", after=mark)
        time.sleep(0.2)
        t0 = time.monotonic()
        s.send(signal.SIGINT)
        wait_prompt(s, after=s.mark())
        elapsed = time.monotonic() - t0
        require(s, elapsed < 3.0, name, f"WAIT was not interrupted promptly ({elapsed:.1f}s)")
        require(s, "done" not in s.buf, name, "WAIT continued after SIGINT")
        mark = s.mark()
        s.write("print [woke]\n")
        s.wait_for("woke", after=mark)
        s.bye()
    except Exception:
        s.close()
        raise


def test_priority_quit_over_pause(logo_path: str) -> None:
    name = "priority_quit_over_pause"
    s = Session(logo_path)
    try:
        wait_prompt(s)
        s.write("wait 10000\n")
        time.sleep(0.2)
        s.send(signal.SIGTSTP)
        s.send(signal.SIGQUIT)
        status = s.wait_exit()
        require(s, CATCH_TAG_MSG not in s.buf, name, "priority test printed catch-tag error")
        require(s, status >= 0, name, f"process died from a signal ({-status})")
        require(s, "Already Pausing" not in s.buf, name, "PAUSE overwrote a pending SYSTEM")
    except Exception:
        s.close()
        raise


TESTS = [
    test_sigint_forever,
    test_catch_toplevel,
    test_sigquit_exits,
    test_catch_system,
    test_sigtstp_procedure,
    test_continue_value,
    test_already_pausing,
    test_sigint_during_pause,
    test_sigint_at_prompt,
    test_sigint_during_to,
    test_sigint_during_wait,
    test_priority_quit_over_pause,
]


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: run_signals.py /path/to/qlogo", file=sys.stderr)
        return 2
    logo_path = sys.argv[1]
    failed: list[str] = []
    for fn in TESTS:
        name = fn.__name__.removeprefix("test_")
        print(f"signals: {name}")
        try:
            fn(logo_path)
        except Failed as exc:
            print(f"  FAIL: {exc.message}")
            print("  transcript:")
            print(exc.transcript)
            failed.append(name)
        except Timeout as exc:
            print(f"  FAIL: {exc}")
            failed.append(name)
        except Exception as exc:
            print(f"  FAIL: {type(exc).__name__}: {exc}")
            failed.append(name)
    if failed:
        print("failed:", ", ".join(failed))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
