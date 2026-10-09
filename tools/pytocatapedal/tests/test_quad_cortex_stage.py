from pytocatapedal.quad_cortex import STAGE_HW_INPUT, STAGE_HW_OUTPUT, build_stage_user

BLUE, RED = 0xFF0000FF, 0xFFFF0000

QC_DATA = {
    "setlists": [
        {
            "key": "s1", "name": "Gig", "bank": 2,
            "presets": [
                {"name": "Clean", "index": 0, "default_scene": 1,
                 "scene_labels": ["Intro", "", "", "", "", "", "", "Solo"],
                 "scene_colors": [BLUE] * 7 + [RED]},
                {"name": "Lead", "index": 1, "default_scene": 0,
                 "scene_labels": ["A", "B", "C", "Busy", "", "", "", ""],
                 "scene_colors": [RED] * 8},
            ],
        },
    ],
}


def test_stage_user_shape():
    user = build_stage_user(QC_DATA, "Quad", program_mode_switch=3)
    assert user["name"] == "Quad"
    assert [p["name"] for p in user["presets"]] == ["Clean", "Lead"]
    assert user["setlists"][0]["name"] == "Gig"
    assert user["setlists"][0]["presetIds"] == [p["id"] for p in user["presets"]]

    clean = user["presets"][0]
    assert [e["audioUnitId"] for e in clean["effects"]] == [STAGE_HW_INPUT, STAGE_HW_OUTPUT]
    assert clean["connections"][0]["from"] == clean["effects"][0]["id"]
    assert clean["connections"][0]["to"] == clean["effects"][1]["id"]
    assert clean["sceneIndex"] == 1
    scenes = clean["scenes"]
    assert len(scenes) == 8
    assert scenes[0] == {"name": "Intro", "color": "blue", "mode": "scene"}
    assert scenes[1]["name"] == "DEFAULT"
    assert scenes[2]["name"] == ""
    assert scenes[3] == {"name": "...", "color": "none", "mode": "preset"}
    assert scenes[7] == {"name": "Solo", "color": "red", "mode": "scene"}

    # A labeled scene is never replaced by the program-mode switch.
    assert user["presets"][1]["scenes"][3]["mode"] == "scene"


def test_stage_user_id_is_stable_per_name():
    assert build_stage_user(QC_DATA, "Quad")["id"] == build_stage_user(QC_DATA, "Quad")["id"]
    assert build_stage_user(QC_DATA, "Quad")["id"] != build_stage_user(QC_DATA, "Other")["id"]
