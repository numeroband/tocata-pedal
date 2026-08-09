from pytocatapedal.qc_bridge import _lround, freq_to_midi_note


def test_no_pitch_is_note_zero_velocity_zero():
    assert freq_to_midi_note(0) == (0, 0)


def test_a4_reference_pitch_is_note_69_velocity_zero():
    assert freq_to_midi_note(440.0) == (69, 0)


def test_flat_pitch_decrements_note_and_biases_velocity_high():
    # -30 cents from A4 (69.0 - 0.3 semitones): velocity_signed = -60, which
    # the negative branch turns into note=68, velocity=-60+128=68.
    freq = 440.0 * 2 ** (-0.3 / 12)
    assert freq_to_midi_note(freq) == (68, 68)


def test_sharp_pitch_keeps_note_and_uses_positive_velocity():
    # +25 cents from A4: velocity_signed = 50, positive branch, note unchanged.
    freq = 440.0 * 2 ** (0.25 / 12)
    assert freq_to_midi_note(freq) == (69, 50)


def test_lround_rounds_half_away_from_zero_unlike_builtin_round():
    # Python's round() uses banker's rounding (round(2.5) == 2); std::lround
    # (and this helper) rounds every .5 away from zero, matching the C++.
    assert round(2.5) == 2  # sanity check on the builtin's behavior
    assert _lround(2.5) == 3
    assert _lround(-2.5) == -3
    assert _lround(0.5) == 1
    assert _lround(-0.5) == -1
