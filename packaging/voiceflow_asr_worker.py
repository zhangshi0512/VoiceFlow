"""VoiceFlow ASR worker process using moonshine-voice."""

from __future__ import annotations

import sys
import threading
import time

try:
    from moonshine_voice import MicTranscriber, ModelArch, TranscriptEventListener, get_model_for_language
except ImportError:
    print("ERROR: moonshine-voice is not installed", flush=True)
    sys.exit(1)


class LineCollector(TranscriptEventListener):
    def __init__(self) -> None:
        self._final_lines: list[str] = []
        self._lock = threading.Lock()

    def on_line_completed(self, event) -> None:
        text = getattr(event.line, "text", "").strip()
        if text:
            with self._lock:
                self._final_lines.append(text)

    def take_last(self) -> str:
        with self._lock:
            if not self._final_lines:
                return ""
            return self._final_lines[-1]

    def clear(self) -> None:
        with self._lock:
            self._final_lines.clear()


class Session:
    def __init__(self) -> None:
        self._transcriber: MicTranscriber | None = None
        self._collector = LineCollector()

    def init(self, language: str, force_cpu: bool) -> None:
        del force_cpu  # GPU EP selection can be added later.
        model_path, model_arch = get_model_for_language(language)
        arch = ModelArch(model_arch)
        self._transcriber = MicTranscriber(model_path=model_path, model_arch=arch)
        self._transcriber.add_listener(self._collector)

    def start(self) -> None:
        if not self._transcriber:
            raise RuntimeError("Worker not initialized")
        self._collector.clear()
        self._transcriber.start()

    def stop(self) -> str:
        if not self._transcriber:
            raise RuntimeError("Worker not initialized")
        self._transcriber.stop()
        time.sleep(0.2)
        return self._collector.take_last()


def main() -> int:
    session = Session()
    for raw in sys.stdin:
        command = raw.strip()
        if not command:
            continue
        if command == "QUIT":
            break
        if command.startswith("INIT "):
            parts = dict(item.split("=", 1) for item in command.split()[1:] if "=" in item)
            session.init(parts.get("lang", "en"), parts.get("force_cpu", "0") == "1")
            continue
        if command == "START":
            session.start()
            continue
        if command == "STOP":
            try:
                text = session.stop()
            except Exception as exc:  # noqa: BLE001
                print(f"ERROR: {exc}", flush=True)
                continue
            print(f"FINAL:{text}", flush=True)
            continue
        print(f"ERROR: unknown command {command}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
