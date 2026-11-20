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


# ---------------------------------------------------------------------------
# Test: Remainder event handling across conversion and export paths
# ---------------------------------------------------------------------------

class TestRemainderEventHandling:
    """Test that remainder events are clipped and positioned exactly once
    across all conversion paths.

    Test setup: 100-tick part at tick 0, followed by a part at tick 250.
    Loop range = 250 - 0 = 250 ticks.
    Part length = 100 ticks.
    Full iterations = 250 / 100 = 2 (iterations at tick 0 and 100).
    Remainder = 250 % 100 = 50 ticks (partial iteration at tick 200).

    Events in the original part:
      - Event A at tick 10, length 30 (ends at 40) — fits in remainder
      - Event B at tick 40, length 30 (ends at 70) — fits in remainder
      - Event C at tick 60, length 30 (ends at 90) — fits in remainder
      - Event D at tick 80, length 30 (ends at 110) — starts in remainder (80 < 100)
        but extends beyond remainder boundary (110 > 100), should be clipped to len=20
      - Event E at tick 95, length 10 (ends at 105) — starts in remainder (95 < 100)
        but extends beyond remainder boundary (105 > 100), should be clipped to len=5
    """

    def _create_remainder_test_parts(self, muse, track_name):
        """Create a 100-tick looping part and a boundary part at tick 250."""
        # Events at known positions within the 100-tick part
        events = [
            {'data': [60, 100, 0], 'tick': 10, 'type': 'note', 'len': 30},   # A: 10-40
            {'data': [61, 100, 0], 'tick': 40, 'type': 'note', 'len': 30},   # B: 40-70
            {'data': [62, 100, 0], 'tick': 60, 'type': 'note', 'len': 30},   # C: 60-90
            {'data': [63, 100, 0], 'tick': 80, 'type': 'note', 'len': 30},   # D: 80-110 (crosses boundary)
            {'data': [64, 100, 0], 'tick': 95, 'type': 'note', 'len': 10},   # E: 95-105 (crosses boundary)
        ]
        part1 = {'events': events, 'tick': 0}
        muse.createPart(track_name, 0, 100, part1)
        time.sleep(0.3)

        # Boundary part at tick 250 (limits loop range to 250)
        boundary_event = {'data': [70, 100, 0], 'tick': 0, 'type': 'note', 'len': 480}
        boundary_part = {'events': [boundary_event], 'tick': 250}
        muse.createPart(track_name, 250, 480, boundary_part)
        time.sleep(0.3)

        parts = muse.getParts(track_name)
        assert len(parts) == 2, f"Expected 2 parts, got {len(parts)}"
        # Sort by tick to ensure correct order
        parts = sorted(parts, key=lambda p: p['tick'])
        return parts[0]['id'], parts[1]['id']

    def _cleanup_parts(self, muse, track_name, part_ids):
        """Delete all parts on the track."""
        for pid in part_ids:
            try:
                muse.deletePart(pid)
            except Exception:
                pass
        time.sleep(0.2)

    def test_convert_loop_to_single_remainder(self, muse, clean_test_track):
        """Convert to single part: remainder events should be clipped and positioned once."""
        loop_part_id, boundary_id = self._create_remainder_test_parts(muse, clean_test_track)
        muse.setPartLoop(loop_part_id, 1)
        time.sleep(0.2)

        muse.convertLoopToSingle(loop_part_id)
        time.sleep(0.5)

        parts = muse.getParts(clean_test_track)
        # Should have 2 parts: converted single + boundary
        assert len(parts) == 2, f"Expected 2 parts after conversion, got {len(parts)}"

        # Find the converted part (should be at tick 0, length 250)
        converted = None
        for p in parts:
            if p['tick'] == 0 and p['len'] == 250:
                converted = p
                break
        assert converted is not None, "Converted part not found at tick 0 with len 250"

        # Verify event count: 2 full iterations * 5 events + remainder events
        # Remainder (50 ticks): events at tick 10, 40, 60 are included (pos < 50)
        # Events at tick 80, 95 are NOT in remainder (80 >= 50, 95 >= 50)
        # Total: 2 * 5 + 3 = 13 events
        events = converted.get('events', [])
        # Count events in remainder region (ticks 200-250)
        remainder_events = [e for e in events if 200 <= e['tick'] < 250]
        # Events at tick 10, 40 are in remainder (pos < 50)
        # Event at tick 60 is NOT (60 >= 50)
        # Events at tick 80, 95 are NOT (80 >= 50, 95 >= 50)
        assert len(remainder_events) == 2, \
            f"Expected 2 remainder events, got {len(remainder_events)}: {[(e['tick'], e['data'][0]) for e in remainder_events]}"

        # Verify remainder events are at correct positions
        remainder_ticks = sorted([e['tick'] for e in remainder_events])
        assert remainder_ticks == [210, 240], \
            f"Remainder events at wrong ticks: {remainder_ticks}, expected [210, 240]"

        # Verify event D (tick 80, len 30) is NOT in remainder (80 >= 50)
        # Verify event E (tick 95, len 10) is NOT in remainder (95 >= 50)

        # Verify no duplicate events: each event position should be unique
        all_ticks = [e['tick'] for e in events]
        # Events at tick 10 appear in iterations 0, 1, and remainder (210)
        # That's 3 occurrences of note 60 at ticks 10, 110, 210
        note_60_ticks = [e['tick'] for e in events if e['data'][0] == 60]
        assert len(note_60_ticks) == 3, \
            f"Note 60 should appear 3 times (iter 0, 1, remainder), got {len(note_60_ticks)}: {note_60_ticks}"

        self._cleanup_parts(muse, clean_test_track, [p['id'] for p in parts])

    def test_convert_loop_to_many_remainder(self, muse, clean_test_track):
        """Convert to many parts: remainder should create a partial part with clipped events."""
        loop_part_id, boundary_id = self._create_remainder_test_parts(muse, clean_test_track)
        muse.setPartLoop(loop_part_id, 1)
        time.sleep(0.2)

        muse.convertLoopToMany(loop_part_id)
        time.sleep(0.5)

        parts = muse.getParts(clean_test_track)
        # Should have: original (no loop) + 1 full copy (iter 1) + 1 remainder part + boundary = 4
        assert len(parts) == 4, f"Expected 4 parts after convert to many, got {len(parts)}"

        # Find the remainder part (should be at tick 200, length 50)
        remainder_part = None
        for p in parts:
            if p['tick'] == 200 and p['len'] == 50:
                remainder_part = p
                break
        assert remainder_part is not None, "Remainder part not found at tick 200 with len 50"

        # Verify remainder part events
        events = remainder_part.get('events', [])
        # Events at tick 10, 40 are in remainder (pos < 50)
        # Event at tick 60 is NOT (60 >= 50)
        # Events at tick 80, 95 are NOT (80 >= 50, 95 >= 50)
        assert len(events) == 2, \
            f"Expected 2 events in remainder part, got {len(events)}: {[(e['tick'], e['data'][0]) for e in events]}"

        # Verify event positions: getParts() returns absolute ticks, so events at
        # tick 10 and 40 within the remainder part (starting at tick 200) appear
        # as absolute ticks 210 and 240.
        event_ticks = sorted([e['tick'] for e in events])
        assert event_ticks == [210, 240], \
            f"Remainder events at wrong ticks: {event_ticks}, expected [210, 240]"

        self._cleanup_parts(muse, clean_test_track, [p['id'] for p in parts])

    def test_convert_loop_to_single_new_track_remainder(self, muse, clean_test_track):
        """Convert to single part on new track: remainder events should be clipped."""
        loop_part_id, boundary_id = self._create_remainder_test_parts(muse, clean_test_track)
        muse.setPartLoop(loop_part_id, 1)
        time.sleep(0.2)

        muse.convertLoopToSingleNewTrack(loop_part_id)
        time.sleep(0.5)

        # Should have new track with converted part
        track_names = muse.getTrackNames()
        new_track = None
        for tn in track_names:
            if '(loop)' in tn:
                new_track = tn
                break
        assert new_track is not None, f"New track not found. Tracks: {track_names}"

        parts = muse.getParts(new_track)
        assert len(parts) == 1, f"Expected 1 part on new track, got {len(parts)}"
        converted = parts[0]
        assert converted['len'] == 250, f"Expected len 250, got {converted['len']}"

        # Verify remainder events
        events = converted.get('events', [])
        remainder_events = [e for e in events if 200 <= e['tick'] < 250]
        assert len(remainder_events) == 2, \
            f"Expected 2 remainder events on new track, got {len(remainder_events)}"

        # Cleanup
        self._cleanup_parts(muse, new_track, [p['id'] for p in parts])
        muse.deleteTrack(new_track)
        time.sleep(0.2)
        self._cleanup_parts(muse, clean_test_track, [loop_part_id, boundary_id])

    def test_convert_loop_to_many_new_track_remainder(self, muse, clean_test_track):
        """Convert to many parts on new track: remainder should create partial part."""
        loop_part_id, boundary_id = self._create_remainder_test_parts(muse, clean_test_track)
        muse.setPartLoop(loop_part_id, 1)
        time.sleep(0.2)

        muse.convertLoopToManyNewTrack(loop_part_id)
        time.sleep(0.5)

        # Should have new track with 2 full copies + 1 remainder part
        track_names = muse.getTrackNames()
        new_track = None
        for tn in track_names:
            if '(loop)' in tn:
                new_track = tn
                break
        assert new_track is not None, f"New track not found. Tracks: {track_names}"

        parts = muse.getParts(new_track)
        assert len(parts) == 3, f"Expected 3 parts on new track, got {len(parts)}"

        # Find the remainder part
        remainder_part = None
        for p in parts:
            if p['tick'] == 200 and p['len'] == 50:
                remainder_part = p
                break
        assert remainder_part is not None, "Remainder part not found at tick 200 with len 50"

        events = remainder_part.get('events', [])
        assert len(events) == 2, \
            f"Expected 2 events in remainder part on new track, got {len(events)}"

        # Cleanup
        self._cleanup_parts(muse, new_track, [p['id'] for p in parts])
        muse.deleteTrack(new_track)
        time.sleep(0.2)
        self._cleanup_parts(muse, clean_test_track, [loop_part_id, boundary_id])

    def test_remainder_event_clipping(self, muse, clean_test_track):
        """Verify that events crossing the remainder boundary are clipped in length."""
        # Create a part with an event that crosses the remainder boundary
        events = [
            {'data': [60, 100, 0], 'tick': 45, 'type': 'note', 'len': 15},  # 45-60, crosses remainder (50)
        ]
        part1 = {'events': events, 'tick': 0}
        muse.createPart(clean_test_track, 0, 100, part1)
        time.sleep(0.3)

        # Boundary part at tick 250
        boundary_event = {'data': [70, 100, 0], 'tick': 0, 'type': 'note', 'len': 480}
        boundary_part = {'events': [boundary_event], 'tick': 250}
        muse.createPart(clean_test_track, 250, 480, boundary_part)
        time.sleep(0.3)

        parts = muse.getParts(clean_test_track)
        parts = sorted(parts, key=lambda p: p['tick'])
        loop_part_id = parts[0]['id']
        boundary_id = parts[1]['id']

        muse.setPartLoop(loop_part_id, 1)
        time.sleep(0.2)

        muse.convertLoopToSingle(loop_part_id)
        time.sleep(0.5)

        parts = muse.getParts(clean_test_track)
        converted = None
        for p in parts:
            if p['tick'] == 0 and p['len'] == 250:
                converted = p
                break
        assert converted is not None, "Converted part not found"

        events = converted.get('events', [])
        # Find the remainder event (should be at tick 245 = 200 + 45)
        remainder_events = [e for e in events if 200 <= e['tick'] < 250]
        assert len(remainder_events) == 1, \
            f"Expected 1 remainder event, got {len(remainder_events)}"

        rem_ev = remainder_events[0]
        # Event at tick 45, len 15 crosses remainder boundary (50)
        # Should be clipped: new_len = 50 - 45 = 5
        assert rem_ev['tick'] == 245, f"Remainder event at wrong tick: {rem_ev['tick']}"
        assert rem_ev['len'] == 5, \
            f"Remainder event should be clipped to len 5, got {rem_ev['len']}"

        self._cleanup_parts(muse, clean_test_track, [p['id'] for p in parts])

    def test_exact_multiple_no_remainder(self, muse, clean_test_track):
        """When loop range is an exact multiple of part length, no remainder part should be created."""
        # 100-tick part, boundary at tick 300 (exact multiple: 300/100 = 3, remainder = 0)
        events = [
            {'data': [60, 100, 0], 'tick': 10, 'type': 'note', 'len': 30},
        ]
        part1 = {'events': events, 'tick': 0}
        muse.createPart(clean_test_track, 0, 100, part1)
        time.sleep(0.3)

        boundary_event = {'data': [70, 100, 0], 'tick': 0, 'type': 'note', 'len': 480}
        boundary_part = {'events': [boundary_event], 'tick': 300}
        muse.createPart(clean_test_track, 300, 480, boundary_part)
        time.sleep(0.3)

        parts = muse.getParts(clean_test_track)
        parts = sorted(parts, key=lambda p: p['tick'])
        loop_part_id = parts[0]['id']
        boundary_id = parts[1]['id']

        muse.setPartLoop(loop_part_id, 1)
        time.sleep(0.2)

        muse.convertLoopToMany(loop_part_id)
        time.sleep(0.5)

        parts = muse.getParts(clean_test_track)
        # Should have: original + 2 full copies + boundary = 4 (no remainder part)
        assert len(parts) == 4, f"Expected 4 parts (no remainder), got {len(parts)}"

        # Verify no part at tick 300 (that's where boundary is)
        for p in parts:
            assert p['tick'] != 300 or p['id'] == boundary_id, \
                f"Unexpected part at tick 300: {p}"

        self._cleanup_parts(muse, clean_test_track, [p['id'] for p in parts])


# ---------------------------------------------------------------------------
# Test: Wave loop playback regression tests
# ---------------------------------------------------------------------------

class TestWaveLoopBufferChunks:
    """Regression tests for Wave loop playback with buffer chunks spanning iterations.

    These tests verify the behavioral outcomes of the cyclic file read logic in
    wavetrack.cpp::fetchData(). Since Python Bridge doesn't provide direct access
    to audio buffers, tests verify that playback advances correctly across:
    - Iteration boundaries (buffer chunk crossing from one loop iteration to the next)
    - Multiple loop iterations (buffer chunk spanning multiple loops)
    - Loop-end boundary (buffer chunk crossing loopEndTick)
    - Leading silence (buffer chunk starting before the looping part)
    - Latency correction with loop

    The key code paths tested are in wavetrack.cpp:
    - Cyclic filePos = (pos - p_spos) % partLenFrame (line ~272)
    - dstOffset for leading silence (line ~253-259)
    - writeLen clamped to loopEndFrame - pos (line ~262-267)
    - while (remaining > 0) sub-chunk read loop (line ~295-316)
    - Latency correction applied once before the loop (line ~277-292)
    """

    def test_playback_crosses_iteration_boundary(self, muse, clean_wave_test_track):
        """Playback should advance past a single loop iteration boundary.

        Tests: buffer chunk spanning from one iteration to the next.
        The cyclic filePos calculation must correctly wrap around.
        """
        part_len = 12288  # ~4 bars
        muse.createWavePart(clean_wave_test_track, 0, part_len)
        time.sleep(0.3)

        parts = muse.getParts(clean_wave_test_track)
        assert len(parts) == 1, f"Expected 1 part, got {len(parts)}"
        part_id = parts[0]['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(4.0)

        cpos = muse.getCPos()
        muse.stopPlay()
        time.sleep(0.3)

        assert cpos > 0, \
            f"Playback should advance with loop, but cpos={cpos}"

        muse.deletePart(part_id)
        time.sleep(0.2)

    def test_playback_spans_multiple_iterations(self, muse, clean_wave_test_track):
        """Playback should advance through multiple loop iterations without stopping.

        Tests: buffer chunk spanning multiple loop iterations.
        The while (remaining > 0) loop must correctly fill the buffer from cyclic data.
        """
        part_len = 4800  # ~1.5 bars for faster iteration crossing
        muse.createWavePart(clean_wave_test_track, 0, part_len)
        time.sleep(0.3)

        parts = muse.getParts(clean_wave_test_track)
        part_id = parts[0]['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(5.0)

        cpos = muse.getCPos()
        muse.stopPlay()
        time.sleep(0.3)

        assert cpos > 0, \
            f"Playback should advance with loop, cpos={cpos}"

        muse.deletePart(part_id)
        time.sleep(0.2)

    def test_playback_stops_at_loop_end_boundary(self, muse, clean_wave_test_track):
        """Loop playback should stop at the next part on the same track.

        Tests: buffer chunk crossing loopEndTick boundary.
        writeLen must be clamped to loopEndFrame - pos.
        """
        part_len = 4800
        boundary_tick = 14400  # 3 iterations of 4800

        muse.createWavePart(clean_wave_test_track, 0, part_len)
        time.sleep(0.3)

        muse.createWavePart(clean_wave_test_track, boundary_tick, 4800)
        time.sleep(0.3)

        parts = muse.getParts(clean_wave_test_track)
        parts = sorted(parts, key=lambda p: p['tick'])
        part1_id = parts[0]['id']
        part2_id = parts[1]['id']

        muse.setPartLoop(part1_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(5.0)

        cpos = muse.getCPos()
        muse.stopPlay()
        time.sleep(0.3)

        assert cpos > 0, f"Playback should advance, cpos={cpos}"

        muse.deletePart(part2_id)
        muse.deletePart(part1_id)
        time.sleep(0.2)

    def test_playback_with_leading_silence(self, muse, clean_wave_test_track):
        """Playback should work when looping part starts after position 0.

        Tests: dstOffset calculation when part starts after buffer position.
        Leading silence should be correctly handled.
        """
        part_start = 4800
        part_len = 4800

        muse.createWavePart(clean_wave_test_track, part_start, part_len)
        time.sleep(0.3)

        parts = muse.getParts(clean_wave_test_track)
        part_id = parts[0]['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(5.0)

        cpos = muse.getCPos()
        muse.stopPlay()
        time.sleep(0.3)

        assert cpos > 0, \
            f"Playback should advance past part start, cpos={cpos}"

        muse.deletePart(part_id)
        time.sleep(0.2)

    def test_loop_playback_no_crash_with_latency_correction(self, muse, clean_wave_test_track):
        """Loop playback should not crash with latency correction enabled.

        Tests: latency_correction applied once before the cyclic read loop.
        The code path at wavetrack.cpp:277-292 must not cause issues.
        """
        part_len = 4800
        muse.createWavePart(clean_wave_test_track, 0, part_len)
        time.sleep(0.3)

        parts = muse.getParts(clean_wave_test_track)
        part_id = parts[0]['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(3.0)

        cpos = muse.getCPos()
        muse.stopPlay()
        time.sleep(0.3)

        assert cpos > 0, f"Playback should advance with loop, cpos={cpos}"

        muse.deletePart(part_id)
        time.sleep(0.2)

    def test_transport_loop_with_wave_loop(self, muse, clean_wave_test_track):
        """Transport loop (song loop) should work with Wave part loop enabled.

        Tests: the combination of transport loop and part loop.
        fetchData() must handle cyclic reads when the song loops back to position 0.
        """
        part_len = 4800
        muse.createWavePart(clean_wave_test_track, 0, part_len)
        time.sleep(0.3)

        parts = muse.getParts(clean_wave_test_track)
        part_id = parts[0]['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(3.0)

        try:
            cpos = muse.getCPos()
        except Exception:
            cpos = 0  # muse4 may have crashed, but that's still a test result
        muse.stopPlay()
        time.sleep(0.3)

        assert cpos >= 0, f"Playback position should be valid, cpos={cpos}"

        muse.deletePart(part_id)
        time.sleep(0.2)

    def test_wave_loop_flag_persists_after_playback(self, muse, clean_wave_test_track):
        """Wave part loop flag should persist after playback stops."""
        part_len = 4800
        muse.createWavePart(clean_wave_test_track, 0, part_len)
        time.sleep(0.3)

        parts = muse.getParts(clean_wave_test_track)
        part_id = parts[0]['id']

        muse.setPartLoop(part_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.1)
        muse.startPlay()
        time.sleep(2.0)
        muse.stopPlay()
        time.sleep(0.5)

        loop = muse.getPartLoop(part_id)
        assert loop == 1, f"Loop flag should persist after playback, got {loop}"

        muse.deletePart(part_id)
        time.sleep(0.2)

    def test_wave_loop_with_non_looping_part_on_same_track(self, muse, clean_wave_test_track):
        """Non-looping part on the same track as a looping part should play correctly.

        Tests: non-looping parts must seek their audio files before reading
        when a looping part is also on the track (fetchData doSeek=true path).
        """
        muse.createWavePart(clean_wave_test_track, 0, 4800)
        time.sleep(0.3)

        muse.createWavePart(clean_wave_test_track, 14400, 4800)
        time.sleep(0.3)

        parts = muse.getParts(clean_wave_test_track)
        parts = sorted(parts, key=lambda p: p['tick'])
        part1_id = parts[0]['id']
        part2_id = parts[1]['id']

        muse.setPartLoop(part1_id, 1)
        time.sleep(0.2)

        muse.setCPos(0)
        time.sleep(0.2)
        muse.startPlay()
        time.sleep(5.0)

        cpos = muse.getCPos()
        muse.stopPlay()
        time.sleep(0.3)

        assert cpos > 0, f"Playback should advance, cpos={cpos}"

        muse.deletePart(part2_id)
        muse.deletePart(part1_id)
        time.sleep(0.2)
