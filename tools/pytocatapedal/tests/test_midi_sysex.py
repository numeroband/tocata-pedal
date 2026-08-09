import pytest

from pytocatapedal.midi_sysex import ANY_CHANNEL, NO_CHANNEL, SYSEX_PREFIX, from_sysex, to_sysex


@pytest.mark.parametrize("length", [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 100])
def test_round_trip_no_channel(length):
    buffer = bytes((i * 37 + 5) % 256 for i in range(length))
    sysex = to_sysex(buffer, NO_CHANNEL)
    assert from_sysex(sysex, NO_CHANNEL) == buffer


@pytest.mark.parametrize("length", [0, 1, 7, 8, 9, 50])
@pytest.mark.parametrize("channel", [0, 5, 15, ANY_CHANNEL])
def test_round_trip_with_channel(length, channel):
    buffer = bytes((i * 53 + 3) % 256 for i in range(length))
    sysex = to_sysex(buffer, channel)
    assert from_sysex(sysex, channel) == buffer


def test_all_byte_values_round_trip():
    buffer = bytes(range(256))
    sysex = to_sysex(buffer)
    assert from_sysex(sysex) == buffer


def test_wildcard_requested_channel_matches_specific_stored_channel():
    buffer = b"hello"
    sysex = to_sysex(buffer, channel=3)
    assert from_sysex(sysex, channel=ANY_CHANNEL) == buffer


def test_wildcard_stored_channel_matches_specific_requested_channel():
    buffer = b"hello"
    sysex = to_sysex(buffer, channel=ANY_CHANNEL)
    assert from_sysex(sysex, channel=7) == buffer


def test_mismatched_specific_channels_rejected():
    buffer = b"hello"
    sysex = to_sysex(buffer, channel=3)
    assert from_sysex(sysex, channel=4) is None


def test_bad_prefix_rejected():
    sysex = bytearray(to_sysex(b"hi"))
    sysex[1] = 0xFF
    assert from_sysex(bytes(sysex)) is None


def test_too_short_rejected():
    assert from_sysex(bytes(SYSEX_PREFIX)) is None
