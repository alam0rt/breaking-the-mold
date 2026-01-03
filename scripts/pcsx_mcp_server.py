#!/usr/bin/env python3
"""
PCSX-Redux MCP Server

A Model Context Protocol (MCP) server that exposes PCSX-Redux's REST API as tools
for AI assistants like GitHub Copilot. This enables interactive debugging workflows
during PSX decompilation work.

Usage:
    python3 scripts/pcsx_mcp_server.py [--port PORT] [--host HOST]

The server communicates via stdio using JSON-RPC 2.0 (MCP protocol).
PCSX-Redux must have its web server enabled on localhost:8080 (default).

Enable in PCSX-Redux: Configuration > Emulation > Enable Web Server
"""

import argparse
import json
import struct
import sys
from dataclasses import dataclass
from typing import Any, Callable

import requests

# Default PCSX-Redux web server settings
DEFAULT_HOST = "localhost"
DEFAULT_PORT = 8080

# PSX memory constants
RAM_BASE = 0x80000000
RAM_SIZE_2MB = 0x200000
RAM_SIZE_8MB = 0x800000


@dataclass
class PCSXClient:
    """HTTP client for PCSX-Redux REST API."""

    host: str = DEFAULT_HOST
    port: int = DEFAULT_PORT
    timeout: float = 5.0

    @property
    def base_url(self) -> str:
        return f"http://{self.host}:{self.port}"

    def _get(self, endpoint: str, **params) -> requests.Response:
        """Make GET request to PCSX-Redux API."""
        url = f"{self.base_url}{endpoint}"
        return requests.get(url, params=params, timeout=self.timeout)

    def _post(self, endpoint: str, data: bytes | None = None, **params) -> requests.Response:
        """Make POST request to PCSX-Redux API."""
        url = f"{self.base_url}{endpoint}"
        return requests.post(url, params=params, data=data, timeout=self.timeout)

    # === RAM Operations ===

    def read_ram_raw(self) -> bytes:
        """Dump entire RAM contents."""
        resp = self._get("/api/v1/cpu/ram/raw")
        resp.raise_for_status()
        return resp.content

    def write_ram(self, offset: int, data: bytes) -> None:
        """Write data to RAM at specified offset."""
        resp = self._post(
            "/api/v1/cpu/ram/raw",
            data=data,
            offset=offset,
            size=len(data),
        )
        resp.raise_for_status()

    # === VRAM Operations ===

    def read_vram_raw(self) -> bytes:
        """Dump entire VRAM contents (1024x512 @ 16bpp = 1MB)."""
        resp = self._get("/api/v1/gpu/vram/raw")
        resp.raise_for_status()
        return resp.content

    def write_vram(self, x: int, y: int, width: int, height: int, data: bytes) -> None:
        """Write pixel data to VRAM rectangle."""
        expected_size = width * height * 2
        if len(data) != expected_size:
            raise ValueError(f"Expected {expected_size} bytes for {width}x{height} @ 16bpp, got {len(data)}")
        resp = self._post(
            "/api/v1/gpu/vram/raw",
            data=data,
            x=x,
            y=y,
            width=width,
            height=height,
        )
        resp.raise_for_status()

    # === Execution Control ===

    def get_execution_status(self) -> dict:
        """Get current emulation status."""
        resp = self._get("/api/v1/execution-flow")
        resp.raise_for_status()
        return resp.json()

    def pause(self) -> None:
        """Pause emulation."""
        resp = self._post("/api/v1/execution-flow", function="pause")
        resp.raise_for_status()

    def resume(self) -> None:
        """Resume/start emulation."""
        resp = self._post("/api/v1/execution-flow", function="resume")
        resp.raise_for_status()

    def reset(self, hard: bool = False) -> None:
        """Reset emulator (soft or hard)."""
        reset_type = "hard" if hard else "soft"
        resp = self._post("/api/v1/execution-flow", function="reset", type=reset_type)
        resp.raise_for_status()

    # === CD/Disc Operations ===

    def read_disc_file(self, filename: str) -> bytes:
        """Read a file from the loaded disc image."""
        resp = self._get("/api/v1/cd/files", filename=filename)
        resp.raise_for_status()
        return resp.content

    def patch_disc_file(self, filename: str, data: bytes) -> None:
        """Patch a file on the loaded disc image."""
        resp = self._post("/api/v1/cd/patch", data=data, filename=filename)
        resp.raise_for_status()

    def patch_disc_sector(self, sector: int, data: bytes, mode: str = "GUESS") -> None:
        """Patch disc sectors directly."""
        resp = self._post("/api/v1/cd/patch", data=data, sector=sector, mode=mode)
        resp.raise_for_status()

    # === Symbols ===

    def upload_symbols(self, map_content: str) -> None:
        """Upload symbol map to PCSX-Redux."""
        resp = self._post(
            "/api/v1/assembly/symbols",
            data=map_content.encode("utf-8"),
            function="upload",
        )
        resp.raise_for_status()

    def reset_symbols(self) -> None:
        """Reset all loaded symbols."""
        resp = self._post("/api/v1/assembly/symbols", function="reset")
        resp.raise_for_status()

    # === Cache ===

    def flush_cache(self) -> None:
        """Flush CPU cache."""
        resp = self._post("/api/v1/cpu/cache", function="flush")
        resp.raise_for_status()

    # === Lua Extension Endpoints ===
    # These require loading scripts/mcp_endpoints.lua in PCSX-Redux

    def lua_get_registers(self) -> dict:
        """Get all CPU registers via Lua endpoint."""
        resp = self._get("/api/v1/lua/registers")
        resp.raise_for_status()
        return resp.json()

    def lua_get_breakpoints(self) -> dict:
        """List all breakpoints via Lua endpoint."""
        resp = self._get("/api/v1/lua/breakpoints")
        resp.raise_for_status()
        return resp.json()

    def lua_add_breakpoint(
        self, address: int, bp_type: str = "Exec", width: int = 4, cause: str = "MCP", pause: bool = True
    ) -> dict:
        """Add a breakpoint via Lua endpoint."""
        resp = self._post(
            "/api/v1/lua/breakpoints",
            data=f"action=add&address={address}&type={bp_type}&width={width}&cause={cause}&pause={str(pause).lower()}".encode(),
        )
        resp.raise_for_status()
        return resp.json()

    def lua_remove_breakpoint(self, bp_id: int) -> dict:
        """Remove a breakpoint via Lua endpoint."""
        resp = self._post(
            "/api/v1/lua/breakpoints",
            data=f"action=remove&id={bp_id}".encode(),
        )
        resp.raise_for_status()
        return resp.json()

    def lua_read_struct(self, address: int, format: str, **kwargs) -> dict:
        """Read structured data via Lua endpoint."""
        params = {"address": address, "format": format, **kwargs}
        resp = self._get("/api/v1/lua/struct", **params)
        resp.raise_for_status()
        return resp.json()


class MCPServer:
    """
    Model Context Protocol server over stdio.
    
    Implements JSON-RPC 2.0 protocol for tool invocation.
    """

    def __init__(self, client: PCSXClient):
        self.client = client
        self.tools: dict[str, Callable] = {}
        self._register_tools()

    def _register_tools(self) -> None:
        """Register all available MCP tools."""
        # Core RAM tools
        self.tools["read_ram"] = self._tool_read_ram
        self.tools["write_ram"] = self._tool_write_ram
        self.tools["read_u8"] = self._tool_read_u8
        self.tools["read_u16"] = self._tool_read_u16
        self.tools["read_u32"] = self._tool_read_u32
        self.tools["read_string"] = self._tool_read_string

        # VRAM tools
        self.tools["read_vram"] = self._tool_read_vram
        self.tools["write_vram"] = self._tool_write_vram

        # Execution control
        self.tools["get_status"] = self._tool_get_status
        self.tools["pause"] = self._tool_pause
        self.tools["resume"] = self._tool_resume
        self.tools["reset"] = self._tool_reset

        # Disc tools
        self.tools["read_disc_file"] = self._tool_read_disc_file
        self.tools["patch_disc_file"] = self._tool_patch_disc_file

        # Symbol tools
        self.tools["upload_symbols"] = self._tool_upload_symbols
        self.tools["reset_symbols"] = self._tool_reset_symbols

    # === RAM Tool Implementations ===

    def _tool_read_ram(self, address: int, size: int) -> dict:
        """
        Read bytes from PSX RAM.

        Args:
            address: PSX memory address (0x80000000-0x801FFFFF for 2MB RAM)
            size: Number of bytes to read

        Returns:
            Dictionary with hex dump and raw bytes (base64)
        """
        ram = self.client.read_ram_raw()

        # Convert PSX address to RAM offset
        if address >= RAM_BASE:
            offset = address - RAM_BASE
        else:
            offset = address

        # Mask to physical RAM
        offset = offset & (len(ram) - 1)

        if offset + size > len(ram):
            size = len(ram) - offset

        data = ram[offset : offset + size]

        # Format as hex dump with ASCII
        lines = []
        for i in range(0, len(data), 16):
            chunk = data[i : i + 16]
            hex_part = " ".join(f"{b:02x}" for b in chunk)
            ascii_part = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
            lines.append(f"{address + i:08x}: {hex_part:<48} {ascii_part}")

        import base64

        return {
            "address": f"0x{address:08x}",
            "size": len(data),
            "hex_dump": "\n".join(lines),
            "base64": base64.b64encode(data).decode("ascii"),
        }

    def _tool_write_ram(self, address: int, hex_data: str) -> dict:
        """
        Write bytes to PSX RAM.

        Args:
            address: PSX memory address
            hex_data: Hex string of bytes to write (e.g., "deadbeef")

        Returns:
            Confirmation of write
        """
        data = bytes.fromhex(hex_data.replace(" ", "").replace("0x", ""))

        if address >= RAM_BASE:
            offset = address - RAM_BASE
        else:
            offset = address

        self.client.write_ram(offset, data)
        return {"status": "ok", "address": f"0x{address:08x}", "bytes_written": len(data)}

    def _tool_read_u8(self, address: int) -> dict:
        """Read unsigned 8-bit value from RAM."""
        result = self._tool_read_ram(address, 1)
        import base64

        data = base64.b64decode(result["base64"])
        return {"address": f"0x{address:08x}", "value": data[0], "hex": f"0x{data[0]:02x}"}

    def _tool_read_u16(self, address: int) -> dict:
        """Read unsigned 16-bit little-endian value from RAM."""
        result = self._tool_read_ram(address, 2)
        import base64

        data = base64.b64decode(result["base64"])
        value = struct.unpack("<H", data)[0]
        return {"address": f"0x{address:08x}", "value": value, "hex": f"0x{value:04x}"}

    def _tool_read_u32(self, address: int) -> dict:
        """Read unsigned 32-bit little-endian value from RAM."""
        result = self._tool_read_ram(address, 4)
        import base64

        data = base64.b64decode(result["base64"])
        value = struct.unpack("<I", data)[0]
        return {"address": f"0x{address:08x}", "value": value, "hex": f"0x{value:08x}"}

    def _tool_read_string(self, address: int, max_length: int = 256) -> dict:
        """Read null-terminated string from RAM."""
        result = self._tool_read_ram(address, max_length)
        import base64

        data = base64.b64decode(result["base64"])
        null_pos = data.find(b"\x00")
        if null_pos >= 0:
            data = data[:null_pos]
        try:
            text = data.decode("ascii")
        except UnicodeDecodeError:
            text = data.decode("latin-1")
        return {"address": f"0x{address:08x}", "string": text, "length": len(text)}

    # === VRAM Tool Implementations ===

    def _tool_read_vram(self, x: int = 0, y: int = 0, width: int = 1024, height: int = 512) -> dict:
        """
        Read VRAM contents.

        Args:
            x, y: Top-left corner (default: 0, 0)
            width, height: Size to read (default: full 1024x512)

        Returns:
            Base64 encoded 16bpp pixel data
        """
        vram = self.client.read_vram_raw()

        # Extract requested rectangle
        import base64

        # Full VRAM is 1024x512 @ 16bpp
        row_bytes = 1024 * 2
        extracted = bytearray()
        for row in range(y, min(y + height, 512)):
            start = row * row_bytes + x * 2
            end = start + width * 2
            extracted.extend(vram[start:end])

        return {
            "x": x,
            "y": y,
            "width": width,
            "height": height,
            "size_bytes": len(extracted),
            "base64": base64.b64encode(bytes(extracted)).decode("ascii"),
        }

    def _tool_write_vram(
        self, x: int, y: int, width: int, height: int, base64_data: str
    ) -> dict:
        """Write 16bpp pixel data to VRAM rectangle."""
        import base64

        data = base64.b64decode(base64_data)
        self.client.write_vram(x, y, width, height, data)
        return {"status": "ok", "x": x, "y": y, "width": width, "height": height}

    # === Execution Control Tools ===

    def _tool_get_status(self) -> dict:
        """Get current emulator execution status."""
        try:
            return self.client.get_execution_status()
        except requests.RequestException as e:
            return {"error": f"Failed to connect to PCSX-Redux: {e}"}

    def _tool_pause(self) -> dict:
        """Pause emulation."""
        self.client.pause()
        return {"status": "paused"}

    def _tool_resume(self) -> dict:
        """Resume emulation."""
        self.client.resume()
        return {"status": "running"}

    def _tool_reset(self, hard: bool = False) -> dict:
        """Reset emulator."""
        self.client.reset(hard=hard)
        return {"status": "reset", "type": "hard" if hard else "soft"}

    # === Disc Tools ===

    def _tool_read_disc_file(self, filename: str) -> dict:
        """
        Read a file from the loaded disc image.

        Args:
            filename: ISO9660 filename (uppercase, ends with ;1)
        """
        import base64

        data = self.client.read_disc_file(filename)
        return {
            "filename": filename,
            "size": len(data),
            "base64": base64.b64encode(data).decode("ascii"),
        }

    def _tool_patch_disc_file(self, filename: str, base64_data: str) -> dict:
        """Patch a file on the loaded disc image."""
        import base64

        data = base64.b64decode(base64_data)
        self.client.patch_disc_file(filename, data)
        return {"status": "ok", "filename": filename, "bytes_written": len(data)}

    # === Symbol Tools ===

    def _tool_upload_symbols(self, symbols: dict[str, int] | str) -> dict:
        """
        Upload symbols to PCSX-Redux.

        Args:
            symbols: Either a dict mapping names to addresses, or a .map file content string
        """
        if isinstance(symbols, dict):
            # Convert dict to map format
            lines = [f"{name} {addr:08x}" for name, addr in symbols.items()]
            map_content = "\n".join(lines)
        else:
            map_content = symbols

        self.client.upload_symbols(map_content)
        return {"status": "ok", "symbols_uploaded": map_content.count("\n") + 1}

    def _tool_reset_symbols(self) -> dict:
        """Reset all loaded symbols."""
        self.client.reset_symbols()
        return {"status": "ok"}

    # === MCP Protocol Implementation ===

    def get_tools_schema(self) -> list[dict]:
        """Return MCP tools schema for capability negotiation."""
        return [
            {
                "name": "read_ram",
                "description": "Read bytes from PSX RAM. Returns hex dump and base64 data.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {
                            "type": "integer",
                            "description": "PSX memory address (e.g., 0x80010000)",
                        },
                        "size": {
                            "type": "integer",
                            "description": "Number of bytes to read",
                        },
                    },
                    "required": ["address", "size"],
                },
            },
            {
                "name": "write_ram",
                "description": "Write bytes to PSX RAM.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {"type": "integer", "description": "PSX memory address"},
                        "hex_data": {
                            "type": "string",
                            "description": "Hex string of bytes to write",
                        },
                    },
                    "required": ["address", "hex_data"],
                },
            },
            {
                "name": "read_u8",
                "description": "Read unsigned 8-bit value from RAM.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {"type": "integer", "description": "PSX memory address"},
                    },
                    "required": ["address"],
                },
            },
            {
                "name": "read_u16",
                "description": "Read unsigned 16-bit little-endian value from RAM.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {"type": "integer", "description": "PSX memory address"},
                    },
                    "required": ["address"],
                },
            },
            {
                "name": "read_u32",
                "description": "Read unsigned 32-bit little-endian value from RAM.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {"type": "integer", "description": "PSX memory address"},
                    },
                    "required": ["address"],
                },
            },
            {
                "name": "read_string",
                "description": "Read null-terminated ASCII string from RAM.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {"type": "integer", "description": "PSX memory address"},
                        "max_length": {
                            "type": "integer",
                            "description": "Maximum bytes to read (default: 256)",
                        },
                    },
                    "required": ["address"],
                },
            },
            {
                "name": "read_vram",
                "description": "Read VRAM contents (GPU framebuffer/textures).",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "x": {"type": "integer", "description": "X coordinate (default: 0)"},
                        "y": {"type": "integer", "description": "Y coordinate (default: 0)"},
                        "width": {"type": "integer", "description": "Width (default: 1024)"},
                        "height": {"type": "integer", "description": "Height (default: 512)"},
                    },
                },
            },
            {
                "name": "get_status",
                "description": "Get current emulator execution status (running/paused).",
                "inputSchema": {"type": "object", "properties": {}},
            },
            {
                "name": "pause",
                "description": "Pause emulation.",
                "inputSchema": {"type": "object", "properties": {}},
            },
            {
                "name": "resume",
                "description": "Resume/start emulation.",
                "inputSchema": {"type": "object", "properties": {}},
            },
            {
                "name": "reset",
                "description": "Reset the emulator.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "hard": {
                            "type": "boolean",
                            "description": "Hard reset (power cycle) if true, soft reset otherwise",
                        },
                    },
                },
            },
            {
                "name": "read_disc_file",
                "description": "Read a file from the loaded disc image.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "filename": {
                            "type": "string",
                            "description": "ISO9660 filename (uppercase, e.g., 'GAME.BLB;1')",
                        },
                    },
                    "required": ["filename"],
                },
            },
            {
                "name": "upload_symbols",
                "description": "Upload symbol names to PCSX-Redux debugger.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "symbols": {
                            "description": "Dict of {name: address} or .map file content string",
                        },
                    },
                    "required": ["symbols"],
                },
            },
            {
                "name": "reset_symbols",
                "description": "Clear all loaded symbols from PCSX-Redux.",
                "inputSchema": {"type": "object", "properties": {}},
            },
        ]

    def handle_request(self, request: dict) -> dict | None:
        """Handle incoming JSON-RPC request."""
        method = request.get("method", "")
        req_id = request.get("id")
        params = request.get("params", {})

        # MCP initialization
        if method == "initialize":
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {"tools": {}},
                    "serverInfo": {
                        "name": "pcsx-redux-mcp",
                        "version": "1.0.0",
                    },
                },
            }

        # List available tools
        if method == "tools/list":
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {"tools": self.get_tools_schema()},
            }

        # Call a tool
        if method == "tools/call":
            tool_name = params.get("name", "")
            tool_args = params.get("arguments", {})

            if tool_name not in self.tools:
                return {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "error": {"code": -32601, "message": f"Unknown tool: {tool_name}"},
                }

            try:
                result = self.tools[tool_name](**tool_args)
                return {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "content": [{"type": "text", "text": json.dumps(result, indent=2)}]
                    },
                }
            except requests.RequestException as e:
                return {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "content": [
                            {
                                "type": "text",
                                "text": json.dumps(
                                    {
                                        "error": "PCSX-Redux connection failed",
                                        "details": str(e),
                                        "hint": "Ensure PCSX-Redux is running with web server enabled on port 8080",
                                    }
                                ),
                            }
                        ],
                        "isError": True,
                    },
                }
            except Exception as e:
                return {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "content": [{"type": "text", "text": json.dumps({"error": str(e)})}],
                        "isError": True,
                    },
                }

        # Notifications (no response needed)
        if method == "notifications/initialized":
            return None

        # Unknown method
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "error": {"code": -32601, "message": f"Unknown method: {method}"},
        }

    def run(self) -> None:
        """Run the MCP server, reading from stdin and writing to stdout."""
        # Use line-delimited JSON over stdio
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue

            try:
                request = json.loads(line)
            except json.JSONDecodeError as e:
                response = {
                    "jsonrpc": "2.0",
                    "id": None,
                    "error": {"code": -32700, "message": f"Parse error: {e}"},
                }
                print(json.dumps(response), flush=True)
                continue

            response = self.handle_request(request)
            if response is not None:
                print(json.dumps(response), flush=True)


def main():
    parser = argparse.ArgumentParser(description="PCSX-Redux MCP Server")
    parser.add_argument(
        "--host",
        default=DEFAULT_HOST,
        help=f"PCSX-Redux host (default: {DEFAULT_HOST})",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"PCSX-Redux port (default: {DEFAULT_PORT})",
    )
    args = parser.parse_args()

    client = PCSXClient(host=args.host, port=args.port)
    server = MCPServer(client)

    # Log startup to stderr (stdout is for MCP protocol)
    print(f"PCSX-Redux MCP Server starting...", file=sys.stderr)
    print(f"Connecting to PCSX-Redux at {client.base_url}", file=sys.stderr)

    server.run()


if __name__ == "__main__":
    main()
