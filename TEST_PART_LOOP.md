# MusE Part Loop Automated Tests

Automated tests for the MusE Part Loop feature using Python Bridge (Pyro5).

## Prerequisites

- **muse4** built and installed
- **Python 3** with `pytest` and `Pyro5` packages:
  ```bash
  pip3 install --break-system-packages pytest Pyro5
  ```
- **Pyro5 Nameserver** running
- **muse4** running with Python Bridge enabled (`-y` flag)

## Running the Tests

### 1. Start Pyro5 Nameserver

```bash
pyro5-ns &
```

### 2. Start muse4 with Python Bridge

```bash
~/CLionProjects/muse/src/cmake-build-debug/muse/muse4 -y &
```

Wait for muse4 to fully load (you should see "Start application loop..." in the output).

### 3. Run the Tests

```bash
cd ~/CLionProjects/muse
pytest test_part_loop.py -v
```

### 4. Run Specific Test Classes

```bash
# Only loop flag tests
pytest test_part_loop.py -v -k "TestPartLoopFlag"

# Only playback tests
pytest test_part_loop.py -v -k "TestLoopPlayback"

# Only edge cases
pytest test_part_loop.py -v -k "TestEdgeCases"
```

## Test Coverage

| Test Class | Tests | Description |
|---|---|---|
| `TestPartLoopFlag` | 4 | Basic loop flag get/set, toggle, persistence |
| `TestLoopWithMultipleParts` | 2 | Loop boundary at next part, isolation between parts |
| `TestLoopPlayback` | 2 | Playback advances with loop, stop halts advancement |
| `TestWaveTrackLoop` | 2 | Wave track creation and loop flag support |
| `TestSongState` | 3 | Song length, tempo, and part count unchanged by loop toggle |
| `TestEdgeCases` | 3 | Empty track, flag persistence after playback, rapid toggles |

**Total: 15 tests**

## Test Design

- **Isolation**: Each test creates its own temporary track and parts, then cleans up.
- **No side effects**: Tests don't modify the loaded project's existing tracks.
- **Session-scoped connection**: The Pyro5 connection is established once per test session.
- **Function-scoped fixtures**: Tracks and parts are created fresh for each test.

## Adding New Tests

To add a new test:

1. Add a new test method to an existing class, or create a new class.
2. Use the `muse` fixture to access the Python Bridge.
3. Use `clean_test_track` for MIDI tracks or `clean_wave_test_track` for Wave tracks.
4. Use `midi_part_on_track` for a pre-created MIDI part with a note event.
5. Clean up any created parts in the test or use the fixture's automatic cleanup.

Example:

```python
def test_my_new_test(self, muse, midi_part_on_track):
    part_id = midi_part_on_track['id']
    muse.setPartLoop(part_id, 1)
    assert muse.getPartLoop(part_id) == 1
```

## Troubleshooting

- **"Cannot connect to muse4 Python Bridge"**: Make sure muse4 is running with `-y` flag and Pyro5 Nameserver is running.
- **Tests hang during playback**: muse4 may not be fully loaded. Wait for "Start application loop..." message.
- **Part creation fails**: The test track name may conflict with existing tracks. The fixtures handle cleanup, but manual cleanup may be needed after a crash.
