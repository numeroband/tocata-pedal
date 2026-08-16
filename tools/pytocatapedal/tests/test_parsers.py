from pytocatapedal.models import (
    Action,
    Color,
    Config,
    ExpressionCal,
    Footswitch,
    FsMode,
    Midi,
    MessageType,
    Mode,
    Program,
    Setlist,
    to_wire,
)
from pytocatapedal.parsers import (
    ACTION_SCHEME,
    NAMES_SCHEME,
    _serialize_buffer,
    parse_addr_payload,
    parse_config,
    parse_names,
    parse_program,
    parse_setlist,
    serialize_addr_payload,
    serialize_config,
    serialize_program,
    serialize_setlist,
)


def test_config_round_trip():
    config = Config(version=3, midi=Midi(channel=7), expression=ExpressionCal(min_raw=100, max_raw=4000))
    data = serialize_config(config)
    assert parse_config(data) == config


def test_config_round_trip_default_expression_calibration():
    # A config with no explicit calibration still round-trips the firmware's
    # defaults, since all 6 bytes of Config are always written (see
    # EXPRESSION_CAL_SCHEME's comment).
    config = Config(version=1, midi=Midi(channel=0))
    data = serialize_config(config)
    assert parse_config(data) == Config(version=1, midi=Midi(channel=0), expression=ExpressionCal())


def test_program_round_trip_with_mixed_footswitches():
    program = Program(
        name="Lead",
        fs=[
            Footswitch(
                on_actions=[Action(type=MessageType.CC, channel=1, values=[10, 20])],
                off_actions=[],
                name="Boost",
                color=Color.RED,
                enabled=True,
                mode=FsMode.STOMP,
            ),
            None,
        ],
        actions=[Action(type=MessageType.PC, channel=0, values=[5, 0])],
        mode=Mode.DEFAULT,
        exp_channel=2,
        expression=1,
    )
    data = serialize_program(4, program)
    id_, parsed = parse_program(data)
    assert id_ == 4
    assert parsed == program


def test_empty_program_slot_parses_to_none():
    data = serialize_program(0, Program(name=""))
    id_, parsed = parse_program(data)
    assert id_ == 0
    assert parsed is None


def test_setlist_round_trip():
    setlist = Setlist(name="Gig", programs=[0, 1, 2, 40])
    data = serialize_setlist(9, setlist)
    id_, parsed = parse_setlist(data)
    assert id_ == 9
    assert parsed == setlist


def test_names_paging_response():
    wire = {"id": 32, "names": ["a", "b", ""]}
    data = _serialize_buffer(wire, NAMES_SCHEME)
    result = parse_names(data)
    assert result["id"] == 32
    assert result["names"][:3] == ["a", "b", ""]


def test_addr_payload_round_trip():
    data = serialize_addr_payload(0x1000, b"\x01\x02\x03")
    addr, payload = parse_addr_payload(data)
    assert addr == 0x1000
    assert payload == b"\x01\x02\x03"


def test_type_and_channel_compact_packing():
    # type=CC (index 2, bits 0-2), globalChannel=0 (bit 3), channel=5 (bits 4-7)
    # -> byte 0x52. globalChannel=0 happens to leave this identical to the old
    # 4-bit-type/4-bit-channel packing, since CC's index fits in 3 bits.
    action = Action(type=MessageType.CC, channel=5, values=[0, 0])
    data = _serialize_buffer(to_wire(action), ACTION_SCHEME)
    assert data[0] == 0x52


def test_type_and_channel_compact_packing_global():
    # type=PC (index 1), globalChannel=1 (bit 3 set) -> byte 0x09, regardless of
    # the explicit channel value underneath (kept at its default here).
    action = Action(type=MessageType.PC, global_channel=1, values=[0, 0])
    data = _serialize_buffer(to_wire(action), ACTION_SCHEME)
    assert data[0] == 0x09


def test_action_global_channel_round_trip():
    # The explicit channel underneath a set globalChannel flag is preserved on
    # the wire (so unsetting the flag later restores it), even though the
    # firmware ignores it while the flag is set.
    action = Action(type=MessageType.CC, global_channel=1, channel=5, values=[10, 20])
    data = _serialize_buffer(to_wire(action), ACTION_SCHEME)
    assert data[0] == 0x02 | 0x08 | (5 << 4)


def test_mode_and_channel_compact_packing():
    # mode=scene (index 1), expGlobalChannel=0 (bit 3), expChannel=9 -> byte 0x91
    program = Program(name="X", mode=Mode.SCENE, exp_channel=9)
    data = serialize_program(0, program)
    # idPlusProgram layout: id(1) + name(31) + fs(1 + 8*44=353) + actions(1 + 5*3=16) + modeAndChannel(1) + expression(1)
    offset = 1 + 31 + 353 + 16
    assert data[offset] == 0x91


def test_mode_and_channel_compact_packing_global():
    # mode=default (index 0), expGlobalChannel=1 (bit 3 set) -> byte 0x08
    program = Program(name="X", mode=Mode.DEFAULT, exp_global_channel=1, exp_channel=9)
    data = serialize_program(0, program)
    offset = 1 + 31 + 353 + 16
    assert data[offset] == 0x08 | (9 << 4)


def test_enum_unmatched_value_wraps_to_255():
    # A footswitch mode with no matching entry in FS_MODES serializes as 0xFF
    # (JS's Uint8Array wraparound for findIndex()==-1) -- relied on elsewhere
    # (see web/CLAUDE.md's Quad Cortex sync notes) so it must round-trip the
    # same way here.
    from pytocatapedal.parsers import FOOTSWITCH_SCHEME

    wire = {"name": "X", "mode": "not-a-real-mode"}
    data = _serialize_buffer(wire, FOOTSWITCH_SCHEME)
    mode_offset = 16 + 16 + 9 + 1 + 1  # onActions + offActions + name + color + enabled
    assert data[mode_offset] == 0xFF
