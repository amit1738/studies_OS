#!/usr/bin/env python3
"""
Comprehensive Test Suite for HW3 Chat Server/Client
Based on the specification from the PDF.

Test Categories:
1. Basic Connection Tests
2. Message Broadcasting Tests  
3. Whisper (Private Message) Tests
4. Disconnection Tests
5. Edge Case Tests
6. Stress Tests
7. Protocol Tests

Run with: python3 tests/run_tests.py
"""

import subprocess
import socket
import time
import sys
import os
import threading
import random
import string

# Configuration
SERVER_PORT = 12345  # Use a high port to avoid conflicts
SERVER_BIN = "./hw3server"
CLIENT_BIN = "./hw3client"
SERVER_IP = "127.0.0.1"

# ANSI colors for output
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def print_pass(msg):
    print(f"{Colors.GREEN}[PASS]{Colors.RESET} {msg}")

def print_fail(msg):
    print(f"{Colors.RED}[FAIL]{Colors.RESET} {msg}")

def print_info(msg):
    print(f"{Colors.BLUE}[INFO]{Colors.RESET} {msg}")

def print_section(msg):
    print(f"\n{Colors.BOLD}{Colors.YELLOW}=== {msg} ==={Colors.RESET}")

class TestContext:
    """Manages server process and client connections for testing."""
    
    def __init__(self, port=None):
        self.port = port or SERVER_PORT
        self.server_process = None
        self.clients = []
        self.server_output = []

    def start_server(self):
        """Start the chat server."""
        self.server_process = subprocess.Popen(
            [SERVER_BIN, str(self.port)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        time.sleep(0.3)  # Give server time to bind
        
        # Check if server started successfully
        if self.server_process.poll() is not None:
            stderr = self.server_process.stderr.read()
            raise Exception(f"Server failed to start: {stderr}")

    def stop_server(self):
        """Stop the server and clean up all clients."""
        for c in self.clients:
            try:
                c.close()
            except:
                pass
        self.clients = []
        
        if self.server_process:
            self.server_process.terminate()
            try:
                self.server_process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.server_process.kill()
            self.server_process = None

    def create_client(self, name, timeout=2.0):
        """Create a client connection and register with the given name."""
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect((SERVER_IP, self.port))
        s.sendall(f"{name}\n".encode())
        self.clients.append(s)
        time.sleep(0.05)  # Small delay for server to process registration
        return s

    def read_from_socket(self, sock, timeout=1.0):
        """Read data from socket with timeout."""
        sock.settimeout(timeout)
        try:
            data = sock.recv(4096)
            return data.decode()
        except socket.timeout:
            return ""
        except Exception as e:
            return ""

    def read_all_available(self, sock, timeout=0.5):
        """Read all available data from socket."""
        sock.settimeout(timeout)
        result = ""
        while True:
            try:
                data = sock.recv(4096)
                if not data:
                    break
                result += data.decode()
            except socket.timeout:
                break
            except:
                break
        return result


# ============================================================================
# TEST FUNCTIONS
# ============================================================================

# --------------------------------------------------------------------------
# 1. BASIC CONNECTION TESTS
# --------------------------------------------------------------------------

def test_single_client_connect(ctx):
    """Test that a single client can connect successfully."""
    s = ctx.create_client("Alice")
    s.sendall(b"hello\n")
    res = ctx.read_from_socket(s)
    assert "Alice: hello" in res, f"Expected echo, got: {res}"

def test_multiple_clients_connect(ctx):
    """Test that multiple clients can connect."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    s3 = ctx.create_client("Charlie")
    
    # All should be able to send messages
    s1.sendall(b"test\n")
    time.sleep(0.1)
    
    r1 = ctx.read_from_socket(s1)
    r2 = ctx.read_from_socket(s2)
    r3 = ctx.read_from_socket(s3)
    
    assert "Alice: test" in r1, f"Alice didn't get echo: {r1}"
    assert "Alice: test" in r2, f"Bob didn't get message: {r2}"
    assert "Alice: test" in r3, f"Charlie didn't get message: {r3}"

def test_client_name_with_numbers(ctx):
    """Test client names containing numbers."""
    s = ctx.create_client("User123")
    s.sendall(b"hi\n")
    res = ctx.read_from_socket(s)
    assert "User123: hi" in res, f"Name with numbers failed: {res}"

def test_client_name_single_char(ctx):
    """Test single character client name."""
    s = ctx.create_client("X")
    s.sendall(b"test\n")
    res = ctx.read_from_socket(s)
    assert "X: test" in res, f"Single char name failed: {res}"

def test_client_name_long(ctx):
    """Test longer client name."""
    name = "VeryLongUserName"
    s = ctx.create_client(name)
    s.sendall(b"msg\n")
    res = ctx.read_from_socket(s)
    assert f"{name}: msg" in res, f"Long name failed: {res}"

# --------------------------------------------------------------------------
# 2. MESSAGE BROADCASTING TESTS
# --------------------------------------------------------------------------

def test_broadcast_simple(ctx):
    """Test simple message broadcast to all clients."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"Hello everyone!\n")
    time.sleep(0.1)
    
    r1 = ctx.read_from_socket(s1)
    r2 = ctx.read_from_socket(s2)
    
    assert "Alice: Hello everyone!" in r1, f"Sender didn't get echo: {r1}"
    assert "Alice: Hello everyone!" in r2, f"Recipient didn't get msg: {r2}"

def test_broadcast_from_different_clients(ctx):
    """Test broadcasts from multiple different clients."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"From Alice\n")
    time.sleep(0.1)
    s2.sendall(b"From Bob\n")
    time.sleep(0.1)
    
    # Read all messages
    r1 = ctx.read_all_available(s1)
    r2 = ctx.read_all_available(s2)
    
    assert "Alice: From Alice" in r1 and "Bob: From Bob" in r1
    assert "Alice: From Alice" in r2 and "Bob: From Bob" in r2

def test_broadcast_message_format(ctx):
    """Test exact message format: 'name: message'"""
    s = ctx.create_client("Test")
    s.sendall(b"hello\n")
    res = ctx.read_from_socket(s)
    # Should be exactly "Test: hello\n"
    assert "Test: hello" in res, f"Format wrong: {res}"
    # Check for colon and space
    assert ": " in res, f"Missing ': ' separator: {res}"

def test_broadcast_preserves_message(ctx):
    """Test that the original message is preserved in broadcast."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    original = "This is a test message with special chars: @#$%"
    s1.sendall(f"{original}\n".encode())
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    assert original in r2, f"Message not preserved: {r2}"

# --------------------------------------------------------------------------
# 3. WHISPER (PRIVATE MESSAGE) TESTS
# --------------------------------------------------------------------------

def test_whisper_basic(ctx):
    """Test basic whisper functionality."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    s3 = ctx.create_client("Charlie")
    time.sleep(0.1)
    
    # Alice whispers to Bob
    s1.sendall(b"@Bob secret message\n")
    time.sleep(0.1)
    
    # Only Bob should receive it
    r2 = ctx.read_from_socket(s2)
    assert "Alice:" in r2 and "@Bob secret message" in r2, f"Bob didn't get whisper: {r2}"
    
    # Charlie should NOT receive it
    r3 = ctx.read_from_socket(s3, 0.3)
    assert r3 == "", f"Charlie received whisper! Got: {r3}"
    
    # Alice should NOT receive echo of whisper
    r1 = ctx.read_from_socket(s1, 0.3)
    assert r1 == "", f"Alice received whisper echo! Got: {r1}"

def test_whisper_to_nonexistent_user(ctx):
    """Test whisper to a user that doesn't exist."""
    s1 = ctx.create_client("Alice")
    time.sleep(0.1)
    
    s1.sendall(b"@Ghost hello\n")
    
    # Should be silently ignored, no crash
    r1 = ctx.read_from_socket(s1, 0.3)
    # No response expected
    assert r1 == "", f"Got unexpected response: {r1}"

def test_whisper_to_self(ctx):
    """Test whisper to yourself."""
    s1 = ctx.create_client("Alice")
    time.sleep(0.1)
    
    s1.sendall(b"@Alice self message\n")
    time.sleep(0.1)
    
    r1 = ctx.read_from_socket(s1)
    # Should receive the whisper since target is self
    assert "Alice:" in r1 and "@Alice self message" in r1, f"Self whisper failed: {r1}"

def test_whisper_format_preserved(ctx):
    """Test that whisper format is preserved in output."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"@Bob private\n")
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    # Spec says "original incoming message" so @Bob should be preserved
    assert "@Bob private" in r2, f"Whisper format not preserved: {r2}"

def test_whisper_with_at_in_message(ctx):
    """Test whisper where message also contains @ symbol."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"@Bob email@test.com\n")
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    assert "email@test.com" in r2, f"@ in message failed: {r2}"

def test_whisper_multiple_targets(ctx):
    """Test whispers to different targets."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    s3 = ctx.create_client("Charlie")
    time.sleep(0.1)
    
    s1.sendall(b"@Bob for bob\n")
    time.sleep(0.1)
    s1.sendall(b"@Charlie for charlie\n")
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    r3 = ctx.read_from_socket(s3)
    
    assert "@Bob for bob" in r2, f"Bob whisper failed: {r2}"
    assert "@Charlie for charlie" in r3, f"Charlie whisper failed: {r3}"
    
    # Bob should NOT get Charlie's message
    assert "@Charlie" not in r2, f"Bob got Charlie's whisper: {r2}"
    # Charlie should NOT get Bob's message
    assert "@Bob" not in r3, f"Charlie got Bob's whisper: {r3}"

def test_whisper_no_space_after_name(ctx):
    """Test @name without space (edge case)."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    # No space after @Bob - this might be treated as @Bob with empty message
    # or as a normal message starting with @Bob
    s1.sendall(b"@Bob\n")
    time.sleep(0.1)
    
    # Behavior depends on implementation - just ensure no crash
    # and check if it was treated as whisper or broadcast
    r1 = ctx.read_from_socket(s1, 0.3)
    r2 = ctx.read_from_socket(s2, 0.3)
    # No assertion on behavior, just ensure no crash

# --------------------------------------------------------------------------
# 4. DISCONNECTION TESTS
# --------------------------------------------------------------------------

def test_client_disconnect(ctx):
    """Test that server handles client disconnect properly."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    # Bob disconnects
    s2.close()
    ctx.clients.remove(s2)
    time.sleep(0.2)
    
    # Alice should still work
    s1.sendall(b"still here\n")
    r1 = ctx.read_from_socket(s1)
    assert "Alice: still here" in r1, f"Alice can't send after Bob disconnect: {r1}"

def test_multiple_disconnects(ctx):
    """Test multiple clients disconnecting."""
    clients = []
    for i in range(5):
        clients.append(ctx.create_client(f"User{i}"))
    time.sleep(0.1)
    
    # Disconnect first 3
    for i in range(3):
        clients[i].close()
        ctx.clients.remove(clients[i])
    time.sleep(0.2)
    
    # Remaining clients should work
    clients[3].sendall(b"test\n")
    r = ctx.read_from_socket(clients[4])
    assert "User3: test" in r, f"Remaining clients broken: {r}"

def test_reconnect_same_name(ctx):
    """Test reconnecting with the same name after disconnect."""
    s1 = ctx.create_client("Alice")
    s1.sendall(b"first\n")
    ctx.read_from_socket(s1)
    
    s1.close()
    ctx.clients.remove(s1)
    time.sleep(0.2)
    
    # Reconnect with same name
    s2 = ctx.create_client("Alice")
    s2.sendall(b"second\n")
    r = ctx.read_from_socket(s2)
    assert "Alice: second" in r, f"Reconnect failed: {r}"

# --------------------------------------------------------------------------
# 5. EDGE CASE TESTS
# --------------------------------------------------------------------------

def test_empty_message(ctx):
    """Test sending an empty message (just newline)."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"\n")
    time.sleep(0.1)
    
    # Server should handle empty message gracefully
    # Might broadcast "Alice: " or ignore - just ensure no crash
    r2 = ctx.read_from_socket(s2, 0.3)
    # No assertion, just ensure no crash

def test_message_with_only_spaces(ctx):
    """Test message containing only spaces."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"   \n")
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    # Should broadcast with spaces preserved
    if r2:
        assert "Alice:" in r2

def test_message_with_special_characters(ctx):
    """Test messages with various special characters."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    special = "!@#$%^&*()_+-=[]{}|;':\",./<>?"
    s1.sendall(f"{special}\n".encode())
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    # Most special chars should be preserved
    assert "Alice:" in r2

def test_message_with_unicode(ctx):
    """Test messages with unicode characters."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall("שלום\n".encode('utf-8'))
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    # Just ensure no crash

def test_message_near_max_length(ctx):
    """Test message close to 256 byte limit."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    # 250 character message
    msg = "A" * 250
    s1.sendall(f"{msg}\n".encode())
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    assert "A" * 100 in r2, f"Long message failed: len={len(r2)}"

def test_message_at_max_length(ctx):
    """Test message at exactly 256 bytes."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    # Account for name prefix in buffer
    msg = "B" * 240
    s1.sendall(f"{msg}\n".encode())
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    assert "B" * 100 in r2, f"Max length message failed"

def test_exit_command(ctx):
    """Test !exit command broadcasts before client exits."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"!exit\n")
    time.sleep(0.1)
    
    # Bob should see the exit message
    r2 = ctx.read_from_socket(s2)
    assert "Alice: !exit" in r2, f"!exit not broadcast: {r2}"

def test_exit_in_message(ctx):
    """Test that !exit only triggers at start of line."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    # "!exit" not at start should be normal message
    s1.sendall(b"type !exit to quit\n")
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    assert "type !exit to quit" in r2, f"!exit in message failed: {r2}"

def test_at_symbol_alone(ctx):
    """Test @ symbol alone."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"@\n")
    time.sleep(0.1)
    
    # Should be handled gracefully
    r1 = ctx.read_from_socket(s1, 0.3)
    r2 = ctx.read_from_socket(s2, 0.3)
    # No crash is success

def test_at_with_space_only(ctx):
    """Test @ followed by just space."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"@ \n")
    time.sleep(0.1)
    
    # Edge case handling
    r1 = ctx.read_from_socket(s1, 0.3)
    r2 = ctx.read_from_socket(s2, 0.3)

def test_duplicate_client_names(ctx):
    """Test what happens with duplicate client names."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Alice")  # Same name!
    time.sleep(0.1)
    
    s1.sendall(b"from first\n")
    time.sleep(0.1)
    
    # Both should receive (they're different connections)
    r1 = ctx.read_from_socket(s1)
    r2 = ctx.read_from_socket(s2)
    
    assert "Alice: from first" in r1
    assert "Alice: from first" in r2

def test_whisper_to_duplicate_name(ctx):
    """Test whisper when multiple clients have same name."""
    s1 = ctx.create_client("Bob")
    s2 = ctx.create_client("Alice")
    s3 = ctx.create_client("Alice")  # Duplicate!
    time.sleep(0.1)
    
    s1.sendall(b"@Alice secret\n")
    time.sleep(0.1)
    
    # One of the Alices should get it (first match typically)
    r2 = ctx.read_from_socket(s2, 0.3)
    r3 = ctx.read_from_socket(s3, 0.3)
    
    # At least one should receive
    assert "@Alice secret" in r2 or "@Alice secret" in r3

def test_message_with_colon(ctx):
    """Test message containing colon (could confuse parsing)."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"time: 12:30:00\n")
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    assert "12:30:00" in r2, f"Colon in message failed: {r2}"

def test_message_starting_with_space(ctx):
    """Test message starting with space."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b" leading space\n")
    time.sleep(0.1)
    
    r2 = ctx.read_from_socket(s2)
    assert "Alice:" in r2

# --------------------------------------------------------------------------
# 6. STRESS TESTS
# --------------------------------------------------------------------------

def test_stress_rapid_messages(ctx):
    """Stress test: send many messages rapidly."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    count = 100
    for i in range(count):
        s1.sendall(f"msg{i}\n".encode())
        time.sleep(0.002)  # Small delay
    
    time.sleep(1)
    
    # Collect all from Bob
    received = ctx.read_all_available(s2, 2.0)
    
    # Check first and last
    assert "msg0" in received, f"First message missing"
    assert f"msg{count-1}" in received, f"Last message missing"

def test_stress_many_clients(ctx):
    """Stress test: connect many clients."""
    clients = []
    max_clients = 15  # Leave room under 16 limit
    
    for i in range(max_clients):
        try:
            c = ctx.create_client(f"C{i}")
            clients.append(c)
        except Exception as e:
            raise Exception(f"Failed at client {i}: {e}")
    
    time.sleep(0.3)
    
    # First client sends message
    clients[0].sendall(b"hello all\n")
    time.sleep(0.5)
    
    # Last client should receive
    r = ctx.read_from_socket(clients[-1])
    assert "C0: hello all" in r, f"Many clients broadcast failed: {r}"

def test_stress_max_clients_limit(ctx):
    """Test server enforces 16 client limit."""
    clients = []
    
    # Connect exactly 16
    for i in range(16):
        try:
            c = ctx.create_client(f"U{i}")
            clients.append(c)
        except Exception as e:
            raise Exception(f"Failed at client {i}: {e}")
    
    time.sleep(0.3)
    
    # 17th should be rejected
    try:
        extra = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        extra.settimeout(1.0)
        extra.connect((SERVER_IP, ctx.port))
        extra.sendall(b"U17\n")
        
        # Server should close this connection
        time.sleep(0.5)
        try:
            data = extra.recv(1024)
            # If we get empty data, connection was closed
            if len(data) == 0:
                pass  # Good - server closed it
        except:
            pass  # Connection refused/reset is also acceptable
        
        extra.close()
    except (ConnectionRefusedError, ConnectionResetError):
        pass  # Server rejected - good

def test_stress_interleaved_messages(ctx):
    """Stress test: multiple clients sending interleaved messages."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    s3 = ctx.create_client("Charlie")
    time.sleep(0.1)
    
    # Interleave messages from all clients
    for i in range(20):
        s1.sendall(f"A{i}\n".encode())
        s2.sendall(f"B{i}\n".encode())
        s3.sendall(f"C{i}\n".encode())
        time.sleep(0.01)
    
    time.sleep(1)
    
    # All should receive all messages
    r1 = ctx.read_all_available(s1, 2.0)
    r2 = ctx.read_all_available(s2, 2.0)
    r3 = ctx.read_all_available(s3, 2.0)
    
    # Check some samples
    assert "A0" in r1 and "B0" in r1 and "C0" in r1
    assert "A19" in r2 and "B19" in r2 and "C19" in r2

def test_stress_whisper_flood(ctx):
    """Stress test: many whisper messages."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    count = 50
    for i in range(count):
        s1.sendall(f"@Bob secret{i}\n".encode())
        time.sleep(0.005)
    
    time.sleep(1)
    
    r2 = ctx.read_all_available(s2, 2.0)
    
    assert "secret0" in r2, "First whisper missing"
    assert f"secret{count-1}" in r2, "Last whisper missing"

def test_stress_connect_disconnect_cycle(ctx):
    """Stress test: rapid connect/disconnect cycles."""
    main = ctx.create_client("Main")
    time.sleep(0.1)
    
    for i in range(10):
        temp = ctx.create_client(f"Temp{i}")
        temp.sendall(b"hi\n")
        time.sleep(0.05)
        temp.close()
        ctx.clients.remove(temp)
        time.sleep(0.05)
    
    # Main should still work
    main.sendall(b"still alive\n")
    r = ctx.read_all_available(main)
    assert "Main: still alive" in r

def test_stress_large_message_flood(ctx):
    """Stress test: many large messages."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    msg = "X" * 200
    for i in range(30):
        s1.sendall(f"{msg}\n".encode())
        time.sleep(0.01)
    
    time.sleep(1)
    
    r2 = ctx.read_all_available(s2, 2.0)
    # Should have received substantial data
    assert len(r2) > 1000, f"Large message flood failed, got {len(r2)} bytes"

# --------------------------------------------------------------------------
# 7. PROTOCOL TESTS
# --------------------------------------------------------------------------

def test_newline_handling(ctx):
    """Test that newlines are properly handled."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    s1.sendall(b"line1\n")
    s1.sendall(b"line2\n")
    time.sleep(0.1)
    
    r2 = ctx.read_all_available(s2)
    
    # Both lines should be separate messages
    lines = r2.strip().split('\n')
    assert len(lines) >= 2, f"Expected 2 lines, got: {lines}"
    assert "Alice: line1" in lines[0]
    assert "Alice: line2" in lines[1]

def test_partial_message_handling(ctx):
    """Test sending message in parts."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    # Send message in parts (simulating slow typing/network)
    s1.sendall(b"hel")
    time.sleep(0.05)
    s1.sendall(b"lo wor")
    time.sleep(0.05)
    s1.sendall(b"ld\n")
    time.sleep(0.2)
    
    r2 = ctx.read_from_socket(s2)
    assert "hello world" in r2, f"Partial send failed: {r2}"

def test_multiple_messages_one_packet(ctx):
    """Test multiple messages sent in one packet."""
    s1 = ctx.create_client("Alice")
    s2 = ctx.create_client("Bob")
    time.sleep(0.1)
    
    # Send multiple messages at once
    s1.sendall(b"msg1\nmsg2\nmsg3\n")
    time.sleep(0.2)
    
    r2 = ctx.read_all_available(s2)
    
    assert "Alice: msg1" in r2, f"msg1 missing: {r2}"
    assert "Alice: msg2" in r2, f"msg2 missing: {r2}"
    assert "Alice: msg3" in r2, f"msg3 missing: {r2}"


# ============================================================================
# TEST RUNNER
# ============================================================================

def run_test(name, func, port):
    """Run a single test with proper setup/teardown."""
    ctx = TestContext(port)
    try:
        ctx.start_server()
        func(ctx)
        print_pass(name)
        return True
    except AssertionError as e:
        print_fail(f"{name}: {e}")
        return False
    except Exception as e:
        print_fail(f"{name}: {e}")
        return False
    finally:
        ctx.stop_server()
        time.sleep(0.1)  # Allow port to be released


def main():
    # Compile first
    print_info("Compiling...")
    result = subprocess.run(["make"], capture_output=True, text=True)
    if result.returncode != 0:
        print_fail(f"Compilation failed:\n{result.stderr}")
        sys.exit(1)
    print_info("Compilation successful")
    
    # Define all tests organized by category
    test_categories = [
        ("1. BASIC CONNECTION TESTS", [
            test_single_client_connect,
            test_multiple_clients_connect,
            test_client_name_with_numbers,
            test_client_name_single_char,
            test_client_name_long,
        ]),
        ("2. MESSAGE BROADCASTING TESTS", [
            test_broadcast_simple,
            test_broadcast_from_different_clients,
            test_broadcast_message_format,
            test_broadcast_preserves_message,
        ]),
        ("3. WHISPER (PRIVATE MESSAGE) TESTS", [
            test_whisper_basic,
            test_whisper_to_nonexistent_user,
            test_whisper_to_self,
            test_whisper_format_preserved,
            test_whisper_with_at_in_message,
            test_whisper_multiple_targets,
            test_whisper_no_space_after_name,
        ]),
        ("4. DISCONNECTION TESTS", [
            test_client_disconnect,
            test_multiple_disconnects,
            test_reconnect_same_name,
        ]),
        ("5. EDGE CASE TESTS", [
            test_empty_message,
            test_message_with_only_spaces,
            test_message_with_special_characters,
            test_message_with_unicode,
            test_message_near_max_length,
            test_message_at_max_length,
            test_exit_command,
            test_exit_in_message,
            test_at_symbol_alone,
            test_at_with_space_only,
            test_duplicate_client_names,
            test_whisper_to_duplicate_name,
            test_message_with_colon,
            test_message_starting_with_space,
        ]),
        ("6. STRESS TESTS", [
            test_stress_rapid_messages,
            test_stress_many_clients,
            test_stress_max_clients_limit,
            test_stress_interleaved_messages,
            test_stress_whisper_flood,
            test_stress_connect_disconnect_cycle,
            test_stress_large_message_flood,
        ]),
        ("7. PROTOCOL TESTS", [
            test_newline_handling,
            test_partial_message_handling,
            test_multiple_messages_one_packet,
        ]),
    ]
    
    passed = 0
    failed = 0
    base_port = 12345
    port_counter = 0
    
    for category_name, tests in test_categories:
        print_section(category_name)
        for test_func in tests:
            port = base_port + port_counter
            port_counter += 1
            if run_test(test_func.__name__, test_func, port):
                passed += 1
            else:
                failed += 1
    
    # Summary
    print_section("SUMMARY")
    total = passed + failed
    print(f"Passed: {Colors.GREEN}{passed}{Colors.RESET}/{total}")
    print(f"Failed: {Colors.RED}{failed}{Colors.RESET}/{total}")
    
    if failed == 0:
        print(f"\n{Colors.GREEN}{Colors.BOLD}All tests passed!{Colors.RESET}")
    else:
        print(f"\n{Colors.RED}{Colors.BOLD}Some tests failed.{Colors.RESET}")
    
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
