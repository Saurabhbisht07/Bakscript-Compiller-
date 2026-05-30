import json
import os
import subprocess
import tempfile
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from typing import Optional

# ── Cloud / Render compatibility ────────────────────────────────────────────
# Render injects PORT as an env var; fall back to 8080 for local dev.
# HOST must be 0.0.0.0 on cloud so the reverse-proxy can reach the process.


ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent


def _find_bakscript(project_root: Path) -> Optional[Path]:
    for name in ("bakscript.exe", "bakscript"):
        p = project_root / name
        if p.is_file():
            return p
    return None


BAKSCRIPT_EXE = _find_bakscript(PROJECT_ROOT)
HOST = "0.0.0.0"
PORT = int(os.environ.get("PORT", 8080))


class BakScriptUIHandler(BaseHTTPRequestHandler):
    def _send_json(self, payload, status=200):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_file(self, path: Path, content_type: str):
        if not path.exists():
            self.send_error(404, "Not Found")
            return
        data = path.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            self._send_file(ROOT / "index.html", "text/html; charset=utf-8")
            return
        if self.path == "/app.js":
            self._send_file(ROOT / "app.js", "application/javascript; charset=utf-8")
            return
        if self.path == "/style.css":
            self._send_file(ROOT / "style.css", "text/css; charset=utf-8")
            return
        if self.path == "/samples":
            samples_dir = PROJECT_ROOT / "tests" / "main"
            names = []
            if samples_dir.exists():
                names = sorted([p.name for p in samples_dir.glob("*.bak")])
            self._send_json({"samples": names})
            return
        if self.path.startswith("/sample/"):
            filename = self.path.split("/sample/", 1)[1]
            if "/" in filename or "\\" in filename:
                self._send_json({"error": "Invalid sample path"}, status=400)
                return
            sample_path = PROJECT_ROOT / "tests" / "main" / filename
            if not sample_path.exists():
                self._send_json({"error": "Sample not found"}, status=404)
                return
            self._send_json({"name": filename, "code": sample_path.read_text(encoding="utf-8")})
            return
        self.send_error(404, "Not Found")

    def do_POST(self):
        if self.path not in ("/run", "/visualize"):
            self.send_error(404, "Not Found")
            return

        exe = BAKSCRIPT_EXE
        if exe is None or not exe.is_file():
            self._send_json(
                {
                    "ok": False,
                    "error": (
                        f"Binary not found in {PROJECT_ROOT} "
                        "(expected bakscript.exe or bakscript). Build with: gcc -o bakscript.exe ..."
                    ),
                },
                status=500,
            )
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
            raw = self.rfile.read(length)
            payload = json.loads(raw.decode("utf-8"))
            code = payload.get("code", "")
        except Exception as exc:
            self._send_json({"ok": False, "error": f"Invalid request: {exc}"}, status=400)
            return

        with tempfile.NamedTemporaryFile(mode="w", suffix=".bak", delete=False, encoding="utf-8") as tmp:
            tmp.write(code)
            tmp_path = tmp.name

        try:
            cmd = [str(exe), tmp_path]
            if self.path == "/visualize":
                cmd = [
                    str(exe),
                    "--debug",
                    "--trace",
                    "--dump-symbols",
                    "--dump-tac",
                    tmp_path,
                ]

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                cwd=str(PROJECT_ROOT),
                timeout=10,
            )
            self._send_json(
                {
                    "ok": result.returncode == 0,
                    "exit_code": result.returncode,
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                }
            )
        except subprocess.TimeoutExpired:
            self._send_json({"ok": False, "error": "Execution timed out"}, status=408)
        finally:
            try:
                os.remove(tmp_path)
            except OSError:
                pass


if __name__ == "__main__":
    server = HTTPServer((HOST, PORT), BakScriptUIHandler)
    print(f"BakScript UI server running at http://{HOST}:{PORT}")
    print("Press Ctrl+C to stop")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
