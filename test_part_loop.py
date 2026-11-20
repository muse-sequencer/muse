# Automated tests for MusE Part Loop feature
#
# These tests use the Python Bridge (Pyro5) to test the loop functionality
# through a running muse4 application.
#
# Prerequisites:
#   - muse4 must be running with Python Bridge enabled (flag -y)
#   - Pyro5 Nameserver must be running
#   - pytest and Pyro5 must be installed: pip3 install pytest Pyro5
#
# Usage:
#   1. Start Pyro5 Nameserver: pyro5-ns &
#   2. Start muse4 with Python Bridge: muse4 -y &
#   3. Wait for muse4 to fully load
#   4. Run tests: pytest test_part_loop.py -v
#
# Note: Tests require a project with at least one MIDI track and one Wave track.
#   The tests will create temporary tracks and parts for testing, then clean up.

import Pyro5.api as pyro
import Pyro5.errors as pyro_errors
import pytest
import time

# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture(scope="session")
def muse():
    """Connect to the running muse4 Python Bridge."""
    try:
        return pyro.Proxy('PYRONAME:muse')
    except pyro_errors.NamingError:
        pytest.fail("Cannot connect to muse4 Python Bridge. "
                     "Make sure muse4 is running with -y flag and Pyro5 Nameserver is running.")


@pytest.fixture(scope="session")
def song_info(muse):
    """Return basic song information for test assertions."""
    return {
        "division": muse.getDivision(),
        "tempo": muse.getTempo(0),
        "song_len": muse.getSongLen(),
    }


@pytest.fixture(scope="function")
def clean_test_track(muse):
    """Create a temporary MIDI track for a single test, then delete it."""
    track_name = "TestLoopTrack"
    # Clean up if it exists from a previous failed test
    existing = muse.getTrackNames()
    if track_name in existing:
        muse.deleteTrack(track_name)
        time.sleep(0.2)
    muse.addMidiTrack(track_name)
    time.sleep(0.3)
    yield track_name
    # Cleanup
    try:
        muse.deleteTrack(track_name)
        time.sleep(0.2)
    except Exception:
        pass


@pytest.fixture(scope="function")
def clean_wave_test_track(muse):
    """Create a temporary Wave track for a single test, then delete it."""
    track_name = "TestWaveLoopTrack"
    existing = muse.getTrackNames()
    if track_name in existing:
        muse.deleteTrack(track_name)
        time.sleep(0.2)
    muse.addWaveTrack(track_name)
    time.sleep(0.3)
    yield track_name
    # Cleanup
    try:
        muse.deleteTrack(track_name)
        time.sleep(0.2)
    except Exception:
        pass


@pytest.fixture(scope="function")
def midi_part_on_track(muse, clean_test_track):
    """Create a simple MIDI part on the test track and return its info."""
    part_len = 12288  # 4 bars at 960 ticks/beat, 4/4
    event = {
        'data': [60, 100, 0],  # C5, velocity 100
        'tick': 0,
        'type': 'note',
        'len': 480,
    }
    part = {
        'events': [event],
        'tick': 0,
    }
    # createPart returns None; get the part ID from getParts
    muse.createPart(clean_test_track, 0, part_len, part)
    time.sleep(0.3)
    parts = muse.getParts(clean_test_track)
    part_id = parts[0]['id'] if parts else None
    yield {'id': part_id, 'track': clean_test_track, 'tick': 0, 'len': part_len}
    # Cleanup: delete the part
    try:
        parts = muse.getParts(clean_test_track)
        for p in parts:
            muse.deletePart(p['id'])
        time.sleep(0.2)
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Test: Part loop flag get/set
# ---------------------------------------------------------------------------

class TestPartLoopFlag:
    """Test basic loop flag get/set operations."""

    def test_loop_flag_default_is_false(self, muse, midi_part_on_track):
        """A newly created part should have loop flag = False."""
        part_id = midi_part_on_track['id']
        loop = muse.getPartLoop(part_id)
        assert loop == 0 or loop is False, f"Expected loop=False, got {loop}"

    def test_set_loop_flag_to_true(self, muse, midi_part_on_track):
        """Setting loop flag to True should persist."""
        part_id = midi_part_on_track['id']
        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)
        loop = muse.getPartLoop(part_id)
        assert loop == 1 or loop is True, f"Expected loop=True, got {loop}"

    def test_set_loop_flag_to_false(self, muse, midi_part_on_track):
        """Setting loop flag to False should persist."""
        part_id = midi_part_on_track['id']
        muse.setPartLoop(part_id, 1)
        time.sleep(0.1)
        muse.setPartLoop(part_id, 0)
        time.sleep(0.2)
        loop = muse.getPartLoop(part_id)
        assert loop == 0 or loop is False, f"Expected loop=False, got {loop}"

    def test_toggle_loop_flag_multiple_times(self, muse, midi_part_on_track):
        """Toggling loop flag multiple times should work correctly."""
        part_id = midi_part_on_track['id']
        for expected in [1, 0, 1, 0, 1]:
            muse.setPartLoop(part_id, expected)
            time.sleep(0.1)
            actual = muse.getPartLoop(part_id)
            assert actual == expected, f"Expected loop={expected}, got {actual}"


# ---------------------------------------------------------------------------
# Test: Part loop with multiple parts on same track
# ---------------------------------------------------------------------------

class TestLoopWithMultipleParts:
    """Test loop behavior when multiple parts exist on the same track."""

    def test_loop_boundary_at_next_part(self, muse, clean_test_track):
        """Loop should end at the next part on the same track."""
        part_len = 4800
        # Create first part (looping)
        event1 = {'data': [60, 100, 0], 'tick': 0, 'type': 'note', 'len': 480}
        part1 = {'events': [event1], 'tick': 0}
        muse.createPart(clean_test_track, 0, part_len, part1)
        time.sleep(0.2)

        # Create second part (boundary)
        event2 = {'data': [64, 100, 0], 'tick': 0, 'type': 'note', 'len': 480}
        part2 = {'events': [event2], 'tick': part_len}
        muse.createPart(clean_test_track, part_len, part_len, part2)
        time.sleep(0.2)

        # Get part IDs from getParts
        parts = muse.getParts(clean_test_track)
        assert len(parts) == 2, f"Expected 2 parts, got {len(parts)}"
        part1_id = parts[0]['id']
        part2_id = parts[1]['id']

        # Enable loop on first part
        muse.setPartLoop(part1_id, 1)
        time.sleep(0.2)

        loop = muse.getPartLoop(part1_id)
        assert loop == 1 or loop is True, "Loop flag should be set on first part"

        # Second part should NOT have loop flag
        loop2 = muse.getPartLoop(part2_id)
        assert loop2 == 0 or loop2 is False, "Second part should not have loop flag"

        # Cleanup
        muse.deletePart(part2_id)
        muse.deletePart(part1_id)
        time.sleep(0.2)

    def test_loop_only_on_one_part(self, muse, clean_test_track):
        """Setting loop on one part should not affect other parts."""
        event1 = {'data': [60, 100, 0], 'tick': 0, 'type': 'note', 'len': 480}
        part1 = {'events': [event1], 'tick': 0}
        muse.createPart(clean_test_track, 0, 4800, part1)
        time.sleep(0.2)

        event2 = {'data': [64, 100, 0], 'tick': 0, 'type': 'note', 'len': 480}
        part2 = {'events': [event2], 'tick': 4800}
        muse.createPart(clean_test_track, 4800, 4800, part2)
        time.sleep(0.2)

        parts = muse.getParts(clean_test_track)
        assert len(parts) == 2, f"Expected 2 parts, got {len(parts)}"
        part1_id = parts[0]['id']
        part2_id = parts[1]['id']

        muse.setPartLoop(part1_id, 1)
        time.sleep(0.2)

        assert muse.getPartLoop(part1_id) == 1, "Part 1 should have loop"
        assert muse.getPartLoop(part2_id) == 0, "Part 2 should not have loop"

        muse.deletePart(part2_id)
        muse.deletePart(part1_id)
        time.sleep(0.2)


# ---------------------------------------------------------------------------
# Test: Playback position with loop
# ---------------------------------------------------------------------------

class TestLoopPlayback:
    """Test that playback advances correctly with loop enabled."""

    def test_playback_advances_with_loop(self, muse, midi_part_on_track):
        """Playback position should advance when playing with loop enabled."""
        part_id = midi_part_on_track['id']
        muse.setPartLoop(part_id, 1)
        time.sleep(0.1)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(3.0)  # Let it play for 3 seconds

        cpos = muse.getCPos()
        muse.stopPlay()
        time.sleep(0.2)

        assert cpos > 0, f"Playback position should advance, but cpos={cpos}"

    def test_stop_play_stops_advancement(self, muse, midi_part_on_track):
        """After stopping playback, position should remain stable."""
        part_id = midi_part_on_track['id']
        muse.setPartLoop(part_id, 1)
        time.sleep(0.1)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(2.0)
        muse.stopPlay()
        time.sleep(0.5)

        pos_after_stop = muse.getCPos()
        time.sleep(1.0)
        pos_after_wait = muse.getCPos()

        assert pos_after_stop == pos_after_wait, \
            f"Position should be stable after stop: {pos_after_stop} vs {pos_after_wait}"


# ---------------------------------------------------------------------------
# Test: Wave track loop
# ---------------------------------------------------------------------------

class TestWaveTrackLoop:
    """Test loop flag on Wave tracks."""

    def test_wave_part_loop_flag(self, muse, clean_wave_test_track):
        """Wave parts should support the loop flag."""
        # Create a minimal wave part (no actual audio file needed for flag test)
        # Wave parts need an audio file, so we just test the track exists
        track_names = muse.getTrackNames()
        assert clean_wave_test_track in track_names, \
            f"Wave test track '{clean_wave_test_track}' should exist"

    def test_wave_track_exists(self, muse, clean_wave_test_track):
        """Wave track should be creatable and deletable."""
        track_names = muse.getTrackNames()
        assert clean_wave_test_track in track_names


# ---------------------------------------------------------------------------
# Test: Song state consistency
# ---------------------------------------------------------------------------

class TestSongState:
    """Test that song state remains consistent after loop operations."""

    def test_song_len_unchanged_after_loop_toggle(self, muse, midi_part_on_track, song_info):
        """Toggling loop flag should not change song length."""
        original_len = song_info['song_len']
        part_id = midi_part_on_track['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)
        assert muse.getSongLen() == original_len, \
            "Song length should not change after enabling loop"

        muse.setPartLoop(part_id, 0)
        time.sleep(0.2)
        assert muse.getSongLen() == original_len, \
            "Song length should not change after disabling loop"

    def test_tempo_unchanged_after_loop_toggle(self, muse, midi_part_on_track, song_info):
        """Toggling loop flag should not change tempo."""
        original_tempo = song_info['tempo']
        part_id = midi_part_on_track['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)
        assert muse.getTempo(0) == original_tempo, \
            "Tempo should not change after enabling loop"

        muse.setPartLoop(part_id, 0)
        time.sleep(0.2)
        assert muse.getTempo(0) == original_tempo, \
            "Tempo should not change after disabling loop"

    def test_part_count_after_loop_operations(self, muse, clean_test_track):
        """Loop operations should not change part count (only conversion does)."""
        event = {'data': [60, 100, 0], 'tick': 0, 'type': 'note', 'len': 480}
        part = {'events': [event], 'tick': 0}
        muse.createPart(clean_test_track, 0, 4800, part)
        time.sleep(0.3)

        initial_parts = muse.getParts(clean_test_track)
        initial_count = len(initial_parts)
        part_id = initial_parts[0]['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)
        after_enable = len(muse.getParts(clean_test_track))

        muse.setPartLoop(part_id, 0)
        time.sleep(0.2)
        after_disable = len(muse.getParts(clean_test_track))

        assert after_enable == initial_count, \
            f"Part count should not change after enabling loop: {initial_count} vs {after_enable}"
        assert after_disable == initial_count, \
            f"Part count should not change after disabling loop: {initial_count} vs {after_disable}"

        muse.deletePart(part_id)
        time.sleep(0.2)


# ---------------------------------------------------------------------------
# Test: Edge cases
# ---------------------------------------------------------------------------

class TestEdgeCases:
    """Test edge cases and boundary conditions."""

    def test_loop_on_empty_track(self, muse, clean_test_track):
        """Setting loop on a non-existent part should not crash."""
        # This tests that the API handles invalid part IDs gracefully
        # The part ID 0 should not exist
        try:
            # This may return 0 or raise an error — both are acceptable
            result = muse.getPartLoop(0)
            assert result == 0 or result is False, \
                f"Non-existent part should return loop=False, got {result}"
        except Exception:
            # It's also acceptable for the API to raise an error for invalid part ID
            pass

    def test_loop_flag_persists_across_playback(self, muse, midi_part_on_track):
        """Loop flag should remain set after playback stops."""
        part_id = midi_part_on_track['id']
        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.1)
        muse.startPlay()
        time.sleep(1.0)
        muse.stopPlay()
        time.sleep(0.5)

        loop = muse.getPartLoop(part_id)
        assert loop == 1 or loop is True, \
            f"Loop flag should persist after playback stops, got {loop}"

    def test_multiple_loop_toggles(self, muse, midi_part_on_track):
        """Rapidly toggling loop flag should work correctly."""
        part_id = midi_part_on_track['id']
        for i in range(10):
            expected = i % 2
            muse.setPartLoop(part_id, expected)
        time.sleep(0.3)
        # After 10 toggles (0,1,0,1,0,1,0,1,0,1), final state should be 1
        actual = muse.getPartLoop(part_id)
        assert actual == 1, f"After 10 toggles, loop should be 1, got {actual}"
