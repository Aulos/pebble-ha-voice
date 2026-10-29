#!/usr/bin/env python3
"""A tiny stand-in for Home Assistant's Assist API, for local testing.

    scripts/mock-ha.py --port 8123 --token test-token

It answers POST /api/conversation/process the way Home Assistant does, so the
watch and the phone-side JavaScript can be exercised without a real instance.
"""
import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

REPLIES = {
    "kitchen": "Turned on the kitchen light.",
    "temperature": "It is 21 degrees outside.",
    "hello": "Hello. How can I help?",
    # A reply longer than one AppMessage chunk, to exercise reassembly and
    # scrolling on the watch.
    "long": (
        "The living room is warm at 23 degrees, the bedroom is cooler at 19 "
        "degrees, and the kitchen is 21 degrees. The bathroom window is open. "
        "The washing machine finishes in 42 minutes. Two lights are on: the "
        "hall light and the desk lamp. The robot vacuum is docked and charged. "
        "Nothing else needs your attention right now."
    ),
}
# Longer than the watch keeps (1024 bytes), so the truncation path runs.
VERY_LONG_REPLY = " ".join(
    "The hallway sensor reports a temperature of %d degrees." % (18 + i) for i in range(30)
)
DEFAULT_REPLY = "Done."


class Handler(BaseHTTPRequestHandler):
    token = "test-token"
    verbose = False

    def _send(self, status, payload):
        body = json.dumps(payload).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Authorization, Content-Type")
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        self._send(200, {})

    def do_GET(self):
        if self.path.rstrip("/") == "/api":
            self._send(200, {"message": "API running."})
        else:
            self._send(404, {"message": "Not found"})

    def do_POST(self):
        if self.path.rstrip("/") != "/api/conversation/process":
            self._send(404, {"message": "Not found"})
            return

        auth = self.headers.get("Authorization", "")
        if auth != "Bearer " + self.token:
            self._send(401, {"message": "Unauthorized"})
            return

        length = int(self.headers.get("Content-Length", "0"))
        try:
            request = json.loads(self.rfile.read(length) or b"{}")
        except json.JSONDecodeError:
            self._send(400, {"message": "Bad request"})
            return

        text = (request.get("text") or "").strip()
        lowered = text.lower()
        if "verylong" in lowered:
            reply = VERY_LONG_REPLY
        else:
            reply = next((v for k, v in REPLIES.items() if k in lowered), DEFAULT_REPLY)
        if self.verbose:
            print(f"  <- {text!r} -> {reply!r}", flush=True)

        self._send(
            200,
            {
                "response": {
                    "speech": {"plain": {"speech": reply, "extra_data": None}},
                    "response_type": "action_done",
                    "language": request.get("language", "en"),
                    "data": {"success": [], "failed": []},
                },
                "conversation_id": "mock",
            },
        )

    def log_message(self, fmt, *args):
        if self.verbose:
            print("  %s" % (fmt % args), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8123)
    parser.add_argument("--token", default="test-token")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    Handler.token = args.token
    Handler.verbose = args.verbose
    server = ThreadingHTTPServer(("0.0.0.0", args.port), Handler)
    print(f"Mock Home Assistant on http://localhost:{args.port} (token: {args.token})", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
