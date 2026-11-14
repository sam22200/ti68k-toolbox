"""One readable appearance per ordinary monster family, chosen for four greys.

User decision2026-11-14: all three jobs share the selected body and portrait;
retain their actual job names, stats, abilities, offspring and save identities.
These are visual choices after inspecting the three native-size conversions,
not a luminance-score winner. Keep this policy shared by later art builders.
"""
REPRESENTATIVE_JOBS = (
    94,   # Chocobo: light face/chest, visible eye; avoid the black silhouette
    97,   # Goblin: separated face, hand and tunic
    100,  # Bomb: bright flame/body against its dark lower rim
    104,  # Panther: Cuar's white flank separates the legs and face
    106,  # Squid: Pisco Demon's light face and front tentacle
    109,  # Skeleton: bright skull against dark body
    112,  # Ghost: Ghoul's light sleeves separate the hands
    116,  # Eyeball: Ahriman's bright eye against its dark lower body
    119,  # Bird: Steel Hawk's light wing against its dark body
    123,  # Pig: Wildbow's white face/body keeps the eye and feet dark
    124,  # Tree: Woodman's light crown separates the dark face and trunk
    128,  # Bull: Minitaurus's white horn/chest separate the limbs
    131,  # Morbol: Ochu's light mouth separates the dark tentacles
    133,  # Behemoth: readable light face/portrait; avoid the dark King portrait
    136,  # Dragon: light belly separates the wings and legs
    141,  # Hydra: Tiamat's light necks keep all three dark faces readable
)


def appearance_job(job):
    if not 94 <= job < 142:
        raise ValueError('not an ordinary monster job')
    return REPRESENTATIVE_JOBS[(job-94)//3]
