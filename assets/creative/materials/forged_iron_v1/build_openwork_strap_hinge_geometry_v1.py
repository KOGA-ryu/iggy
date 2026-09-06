#!/usr/bin/env python3
"""Reference-locked geometry compiler for the Met 55.61.58 strap hinge.

This file is intentionally useful before Blender is allowed to build anything:

    python3 assets/creative/materials/forged_iron_v1/\
build_openwork_strap_hinge_geometry_v1.py --plan-only

The pure-Python planning route creates and validates the complete two-leaf
layout, all aperture instances, the quarter-shape derivation, the section
schedule, and the component ownership map.

The Blender route is present for review but remains locked by the profile's
``geometry_build_authorized`` flag. Even after that flag is reviewed and
changed, Blender also requires an explicit ``--build`` argument after ``--``.
There is no render route in this script.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Any, Iterable, Sequence


PACKAGE_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = (
    PACKAGE_ROOT
    / "profiles"
    / "openwork_strap_hinge_55_61_58_decomposition_v1.json"
)
OUTPUT_PATH = PACKAGE_ROOT / "output" / "openwork_strap_hinge_geometry_v1.blend"

EPSILON = 1.0e-9
ROUND_DIGITS = 9


@dataclass(frozen=True)
class Vec2:
    x: float
    y: float

    def __add__(self, other: "Vec2") -> "Vec2":
        return Vec2(self.x + other.x, self.y + other.y)

    def __sub__(self, other: "Vec2") -> "Vec2":
        return Vec2(self.x - other.x, self.y - other.y)

    def __mul__(self, scalar: float) -> "Vec2":
        return Vec2(self.x * scalar, self.y * scalar)

    def __truediv__(self, scalar: float) -> "Vec2":
        return Vec2(self.x / scalar, self.y / scalar)

    def dot(self, other: "Vec2") -> float:
        return self.x * other.x + self.y * other.y

    def cross(self, other: "Vec2") -> float:
        return self.x * other.y - self.y * other.x

    def length(self) -> float:
        return math.hypot(self.x, self.y)

    def normalized(self) -> "Vec2":
        magnitude = self.length()
        if magnitude <= EPSILON:
            raise ValueError("cannot normalize a zero-length Vec2")
        return self / magnitude

    def rounded_key(self) -> tuple[float, float]:
        return (round(self.x, ROUND_DIGITS), round(self.y, ROUND_DIGITS))


@dataclass(frozen=True)
class Vec3:
    x: float
    y: float
    z: float

    def rounded_tuple(self) -> tuple[float, float, float]:
        return (
            round(self.x, ROUND_DIGITS),
            round(self.y, ROUND_DIGITS),
            round(self.z, ROUND_DIGITS),
        )


@dataclass(frozen=True)
class ProfileVertex:
    point: Vec2
    station_s: float


@dataclass(frozen=True)
class Affine2:
    """Planar transform applied after the canonical leaf is completed."""

    scale_x: float
    scale_y: float
    shear_x_by_y: float = 0.0
    rotation_degrees: float = 0.0
    translation: Vec2 = Vec2(0.0, 0.0)

    def apply(self, point: Vec2) -> Vec2:
        scaled_y = point.y * self.scale_y
        scaled = Vec2(
            point.x * self.scale_x + scaled_y * self.shear_x_by_y,
            scaled_y,
        )
        radians = math.radians(self.rotation_degrees)
        cosine = math.cos(radians)
        sine = math.sin(radians)
        rotated = Vec2(
            scaled.x * cosine - scaled.y * sine,
            scaled.x * sine + scaled.y * cosine,
        )
        return rotated + self.translation


@dataclass(frozen=True)
class SectionSample:
    width_over_nominal: float
    bevel_width_over_local_width: float
    bevel_rise_over_stock_thickness: float
    crown_rise_over_stock_thickness: float
    flat_land_fraction: float


@dataclass(frozen=True)
class Aperture:
    name: str
    shape_id: int
    shape_name: str
    zone_id: int
    cell_index: int
    vertices: tuple[ProfileVertex, ...]
    nominal_bevel: bool = True

    @property
    def points(self) -> tuple[Vec2, ...]:
        return tuple(vertex.point for vertex in self.vertices)


@dataclass(frozen=True)
class PlatePlan:
    name: str
    owner_id: int
    outer: tuple[ProfileVertex, ...]
    apertures: tuple[Aperture, ...]
    thickness_m: float
    outer_bevel_width_m: float
    outer_bevel_drop_m: float


@dataclass(frozen=True)
class KnuckleSegmentPlan:
    name: str
    owner_id: int
    index: int
    cross_min_m: float
    cross_max_m: float
    axis_x_m: float
    outer_radius_m: float
    inner_radius_m: float
    seam_angle_start_degrees: float
    seam_angle_end_degrees: float


@dataclass(frozen=True)
class HingeDesignSpec:
    complete_length_m: float = 4.06
    complete_width_m: float = 0.41
    moving_leaf_length_m: float = 3.16
    fixed_leaf_length_m: float = 0.90
    stock_thickness_m: float = 0.050
    moving_body_start_m: float = 0.048
    pattern_start_m: float = 0.615
    cell_pitch_m: float = 0.370
    cell_count: int = 5
    pattern_half_width_m: float = 0.150
    pivot_pad_end_m: float = 0.505
    tail_transition_start_m: float = 2.465
    tail_pad_start_m: float = 2.600
    tail_pad_end_m: float = 2.980
    terminal_end_m: float = 3.160
    nominal_web_width_m: float = 0.040
    outer_bevel_width_m: float = 0.010
    outer_bevel_drop_m: float = 0.006
    fastener_hole_radius_m: float = 0.027
    terminal_bore_radius_m: float = 0.030
    fixed_tip_half_width_m: float = 0.156
    fixed_root_half_width_m: float = 0.205
    knuckle_outer_radius_m: float = 0.058
    knuckle_inner_radius_m: float = 0.030
    knuckle_count: int = 5
    knuckle_gap_m: float = 0.008
    knuckle_seam_half_angle_degrees: float = 18.0
    pintle_radius_m: float = 0.026
    pintle_length_m: float = 0.410

    @property
    def pattern_end_m(self) -> float:
        return self.pattern_start_m + self.cell_pitch_m * self.cell_count

    @property
    def knuckle_connection_overlap_m(self) -> float:
        seam_x = self.knuckle_outer_radius_m * math.cos(
            math.radians(self.knuckle_seam_half_angle_degrees)
        )
        return seam_x - self.moving_body_start_m

    def validate(self) -> None:
        if not math.isclose(
            self.moving_leaf_length_m + self.fixed_leaf_length_m,
            self.complete_length_m,
            abs_tol=1.0e-8,
        ):
            raise ValueError("fixed and moving leaf spans do not close the catalogue length")
        if not math.isclose(
            self.pattern_end_m,
            self.tail_transition_start_m,
            abs_tol=1.0e-8,
        ):
            raise ValueError("five-cell field does not land on the tail transition")
        if self.terminal_end_m != self.moving_leaf_length_m:
            raise ValueError("tail terminal does not land on the moving-leaf envelope")
        if self.complete_width_m <= self.stock_thickness_m:
            raise ValueError("hinge stock is not visibly flat")
        if self.knuckle_count != 5:
            raise ValueError("the paired reference gate requires five knuckle segments")
        if self.knuckle_inner_radius_m >= self.knuckle_outer_radius_m:
            raise ValueError("knuckle bore must remain open")
        if self.pintle_radius_m >= self.knuckle_inner_radius_m:
            raise ValueError("pintle must preserve radial bearing clearance")
        if self.pintle_length_m != self.complete_width_m:
            raise ValueError("pintle escaped the catalogued cross-stock envelope")
        if self.knuckle_connection_overlap_m < 0.005:
            raise ValueError("knuckle seam does not overlap the owned leaf root")


@dataclass(frozen=True)
class HingePlan:
    profile_sha256: str
    build_authorized: bool
    spec: HingeDesignSpec
    moving_plate: PlatePlan
    fixed_plate: PlatePlan
    knuckles: tuple[KnuckleSegmentPlan, ...]
    shape_counts: dict[str, int]
    minimum_aperture_gap_m: float


@dataclass(frozen=True)
class FaceMeta:
    hinge_zone: int
    cell_index: int = -1
    shape_id: int = 0
    web_station: float = 0.0
    bevel_land: float = 0.0
    aperture_wall: bool = False
    front_face: bool = False
    rear_face: bool = False
    interface_id: int = 0


@dataclass
class MeshPlan:
    name: str
    vertices: list[Vec3] = field(default_factory=list)
    faces: list[tuple[int, ...]] = field(default_factory=list)
    face_meta: list[FaceMeta] = field(default_factory=list)
    _vertex_lookup: dict[tuple[float, float, float], int] = field(
        default_factory=dict,
        repr=False,
    )

    def add_vertex(self, vertex: Vec3) -> int:
        key = vertex.rounded_tuple()
        existing = self._vertex_lookup.get(key)
        if existing is not None:
            return existing
        self.vertices.append(vertex)
        index = len(self.vertices) - 1
        self._vertex_lookup[key] = index
        return index

    def add_face(self, indices: Sequence[int], meta: FaceMeta) -> None:
        if len(indices) < 3:
            raise ValueError("mesh face has fewer than three vertices")
        self.faces.append(tuple(indices))
        self.face_meta.append(meta)

    def validate(self) -> None:
        if len(self.face_meta) != len(self.faces):
            raise ValueError(f"{self.name}: face metadata count is inconsistent")
        if not self.vertices or not self.faces:
            raise ValueError(f"{self.name}: mesh is empty")
        for face in self.faces:
            if len(set(face)) != len(face):
                raise ValueError(f"{self.name}: face repeats a vertex")
            if min(face) < 0 or max(face) >= len(self.vertices):
                raise ValueError(f"{self.name}: face index escaped the vertex buffer")


ZONE_IDS = {
    "fixed_leaf": 1,
    "moving_pivot_pad": 2,
    "moving_pattern": 3,
    "moving_tail_pad": 4,
    "moving_terminal": 5,
    "moving_knuckle": 6,
    "fixed_knuckle": 7,
    "pintle": 8,
}

SHAPE_IDS = {
    "plate_outer": 0,
    "S1_central_lancet_aperture": 1,
    "S2_flank_leaf_aperture": 2,
    "S3_junction_petal_aperture": 3,
    "S4_rail_lune_aperture": 4,
    "S5_terminal_diamond": 5,
    "fastener_hole": 6,
    "terminal_bore": 7,
}

OWNER_IDS = {
    "fixed_leaf": 1,
    "moving_leaf": 2,
    "pintle": 3,
}


def load_profile(path: Path = PROFILE_PATH) -> dict[str, Any]:
    profile = json.loads(path.read_text())
    if profile["schema"] != "iggy-openwork-strap-hinge-decomposition/1.0":
        raise ValueError("unexpected hinge decomposition schema")
    return profile


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def cubic_bezier(
    first: Vec2,
    second: Vec2,
    third: Vec2,
    fourth: Vec2,
    t: float,
) -> Vec2:
    inverse = 1.0 - t
    return (
        first * (inverse ** 3)
        + second * (3.0 * inverse * inverse * t)
        + third * (3.0 * inverse * t * t)
        + fourth * (t ** 3)
    )


def canonical_leaf_vertices(
    profile: dict[str, Any],
    *,
    quarter_steps: int = 16,
) -> tuple[ProfileVertex, ...]:
    """Complete one closed leaf from the single authored Q0 quarter."""

    raw_points = profile["quarter_shape_library"]["canonical_quarter"][
        "control_points"
    ]
    controls = tuple(Vec2(float(x), float(y)) for x, y in raw_points)
    if len(controls) != 4:
        raise ValueError("Q0 must contain exactly four cubic Bezier controls")
    if quarter_steps < 4:
        raise ValueError("Q0 sampling needs at least four quarter steps")

    # The profile records u from tip to belly in [0, 0.5]. Move that into a
    # leaf-centered frame where the right tip is +0.5 and the belly is x=0.
    quarter = []
    for index in range(quarter_steps + 1):
        t = index / quarter_steps
        point = cubic_bezier(*controls, t)
        quarter.append(ProfileVertex(Vec2(0.5 - point.x, point.y), t))

    upper_right_to_left = [
        *quarter,
        *(
            ProfileVertex(Vec2(-vertex.point.x, vertex.point.y), vertex.station_s)
            for vertex in reversed(quarter[:-1])
        ),
    ]
    lower_left_to_right = [
        ProfileVertex(Vec2(vertex.point.x, -vertex.point.y), vertex.station_s)
        for vertex in reversed(upper_right_to_left[1:-1])
    ]
    result = tuple([*upper_right_to_left, *lower_left_to_right])
    return ensure_profile_orientation(result, counter_clockwise=True)


def transform_profile(
    vertices: Sequence[ProfileVertex],
    transform: Affine2,
) -> tuple[ProfileVertex, ...]:
    return ensure_profile_orientation(
        tuple(
            ProfileVertex(transform.apply(vertex.point), vertex.station_s)
            for vertex in vertices
        ),
        counter_clockwise=True,
    )


def polygon_signed_area(points: Sequence[Vec2]) -> float:
    return 0.5 * sum(
        current.cross(points[(index + 1) % len(points)])
        for index, current in enumerate(points)
    )


def ensure_profile_orientation(
    vertices: Sequence[ProfileVertex],
    *,
    counter_clockwise: bool,
) -> tuple[ProfileVertex, ...]:
    result = tuple(vertices)
    area = polygon_signed_area([vertex.point for vertex in result])
    if abs(area) <= EPSILON:
        raise ValueError("profile collapsed to zero area")
    if (area > 0.0) != counter_clockwise:
        result = tuple(reversed(result))
    return result


def profile_from_points(
    points: Sequence[Vec2],
    *,
    station_s: float = 0.5,
) -> tuple[ProfileVertex, ...]:
    return ensure_profile_orientation(
        tuple(ProfileVertex(point, station_s) for point in points),
        counter_clockwise=True,
    )


def circle_profile(
    centre: Vec2,
    radius: float,
    *,
    segments: int = 32,
) -> tuple[ProfileVertex, ...]:
    if radius <= 0.0:
        raise ValueError("circle radius must be positive")
    return profile_from_points(
        [
            centre
            + Vec2(
                radius * math.cos(index * math.tau / segments),
                radius * math.sin(index * math.tau / segments),
            )
            for index in range(segments)
        ]
    )


def interpolate_section(
    profile: dict[str, Any],
    station_s: float,
) -> SectionSample:
    rows = profile["section_grammar"]["pattern_web"]["stations"]
    s = min(1.0, max(0.0, station_s))
    before = rows[0]
    after = rows[-1]
    for first, second in zip(rows, rows[1:]):
        if first["s"] <= s <= second["s"]:
            before, after = first, second
            break
    span = max(float(after["s"]) - float(before["s"]), EPSILON)
    ratio = (s - float(before["s"])) / span

    def lane(name: str) -> float:
        return float(before[name]) + (
            float(after[name]) - float(before[name])
        ) * ratio

    return SectionSample(
        width_over_nominal=lane("width_over_nominal"),
        bevel_width_over_local_width=lane("bevel_width_over_local_width"),
        bevel_rise_over_stock_thickness=lane(
            "bevel_rise_over_stock_thickness"
        ),
        crown_rise_over_stock_thickness=lane(
            "crown_rise_over_stock_thickness"
        ),
        flat_land_fraction=lane("flat_land_fraction"),
    )


def moving_outer_profile(spec: HingeDesignSpec) -> tuple[ProfileVertex, ...]:
    half = spec.complete_width_m * 0.5
    points = (
        Vec2(spec.moving_body_start_m, -half),
        Vec2(spec.tail_pad_end_m, -half),
        Vec2(3.020, -0.158),
        Vec2(3.125, -0.090),
        Vec2(spec.terminal_end_m, 0.0),
        Vec2(3.125, 0.090),
        Vec2(3.020, 0.158),
        Vec2(spec.tail_pad_end_m, half),
        Vec2(spec.moving_body_start_m, half),
    )
    return profile_from_points(points, station_s=0.0)


def fixed_outer_profile(spec: HingeDesignSpec) -> tuple[ProfileVertex, ...]:
    points = (
        Vec2(-spec.fixed_leaf_length_m, -spec.fixed_tip_half_width_m),
        Vec2(-spec.moving_body_start_m, -spec.fixed_root_half_width_m),
        Vec2(-spec.moving_body_start_m, spec.fixed_root_half_width_m),
        Vec2(-spec.fixed_leaf_length_m, spec.fixed_tip_half_width_m),
    )
    return profile_from_points(points, station_s=0.0)


def shape_transform(
    profile: dict[str, Any],
    shape_name: str,
    spec: HingeDesignSpec,
    *,
    centre: Vec2,
    rotation_sign: float = 1.0,
    shear_sign: float = 1.0,
    scale_multiplier: float = 1.0,
) -> Affine2:
    shape = profile["quarter_shape_library"]["derived_shapes"][shape_name]
    scale_x, scale_y = (float(value) for value in shape["scale_in_cell_space"])
    return Affine2(
        scale_x=scale_x * spec.cell_pitch_m * scale_multiplier,
        scale_y=scale_y * spec.pattern_half_width_m * scale_multiplier,
        shear_x_by_y=float(shape["shear"]) * shear_sign,
        rotation_degrees=float(shape["rotation_degrees"]) * rotation_sign,
        translation=centre,
    )


def aperture_from_leaf(
    canonical: Sequence[ProfileVertex],
    profile: dict[str, Any],
    spec: HingeDesignSpec,
    *,
    name: str,
    shape_name: str,
    centre: Vec2,
    zone_id: int,
    cell_index: int,
    rotation_sign: float = 1.0,
    shear_sign: float = 1.0,
    scale_multiplier: float = 1.0,
) -> Aperture:
    return Aperture(
        name=name,
        shape_id=SHAPE_IDS[shape_name],
        shape_name=shape_name,
        zone_id=zone_id,
        cell_index=cell_index,
        vertices=transform_profile(
            canonical,
            shape_transform(
                profile,
                shape_name,
                spec,
                centre=centre,
                rotation_sign=rotation_sign,
                shear_sign=shear_sign,
                scale_multiplier=scale_multiplier,
            ),
        ),
    )


def build_pattern_apertures(
    profile: dict[str, Any],
    spec: HingeDesignSpec,
) -> list[Aperture]:
    canonical = canonical_leaf_vertices(profile)
    apertures: list[Aperture] = []
    pattern_zone = ZONE_IDS["moving_pattern"]

    for cell_index in range(spec.cell_count):
        centre_x = spec.pattern_start_m + (cell_index + 0.5) * spec.cell_pitch_m
        apertures.append(
            aperture_from_leaf(
                canonical,
                profile,
                spec,
                name=f"Cell_{cell_index + 1:02d}_CentralLancet",
                shape_name="S1_central_lancet_aperture",
                centre=Vec2(centre_x, 0.0),
                zone_id=pattern_zone,
                cell_index=cell_index,
                scale_multiplier=0.78,
            )
        )
        for side, sign in (("Lower", -1.0), ("Upper", 1.0)):
            apertures.append(
                aperture_from_leaf(
                    canonical,
                    profile,
                    spec,
                    name=f"Cell_{cell_index + 1:02d}_{side}FlankLeaf",
                    shape_name="S2_flank_leaf_aperture",
                    centre=Vec2(centre_x, sign * 0.096),
                    zone_id=pattern_zone,
                    cell_index=cell_index,
                    rotation_sign=sign,
                    shear_sign=sign,
                    scale_multiplier=0.78,
                )
            )

    for junction_index in range(spec.cell_count - 1):
        centre_x = spec.pattern_start_m + (junction_index + 1) * spec.cell_pitch_m
        for longitudinal_sign in (-1.0, 1.0):
            for cross_sign in (-1.0, 1.0):
                apertures.append(
                    aperture_from_leaf(
                        canonical,
                        profile,
                        spec,
                        name=(
                            f"Junction_{junction_index + 1:02d}_"
                            f"{'Tail' if longitudinal_sign > 0 else 'Pivot'}_"
                            f"{'Upper' if cross_sign > 0 else 'Lower'}Petal"
                        ),
                        shape_name="S3_junction_petal_aperture",
                        centre=Vec2(
                            centre_x + longitudinal_sign * 0.060,
                            cross_sign * 0.066,
                        ),
                        zone_id=pattern_zone,
                        cell_index=junction_index,
                        rotation_sign=longitudinal_sign * cross_sign,
                        shear_sign=cross_sign,
                        scale_multiplier=0.70,
                    )
                )
        for side, cross_sign in (("Lower", -1.0), ("Upper", 1.0)):
            apertures.append(
                aperture_from_leaf(
                    canonical,
                    profile,
                    spec,
                    name=f"Junction_{junction_index + 1:02d}_{side}RailLune",
                    shape_name="S4_rail_lune_aperture",
                    centre=Vec2(centre_x, cross_sign * 0.139),
                    zone_id=pattern_zone,
                    cell_index=junction_index,
                    rotation_sign=cross_sign,
                    shear_sign=cross_sign,
                    scale_multiplier=0.70,
                )
            )
    return apertures


def build_moving_fastener_apertures(spec: HingeDesignSpec) -> list[Aperture]:
    result = []
    pivot_centres = (
        Vec2(0.205, 0.0),
        Vec2(0.390, -0.100),
        Vec2(0.390, 0.100),
    )
    for index, centre in enumerate(pivot_centres, start=1):
        result.append(
            Aperture(
                name=f"PivotFastener_{index:02d}",
                shape_id=SHAPE_IDS["fastener_hole"],
                shape_name="fastener_hole",
                zone_id=ZONE_IDS["moving_pivot_pad"],
                cell_index=-1,
                vertices=circle_profile(centre, spec.fastener_hole_radius_m),
                nominal_bevel=False,
            )
        )
    for index, cross in enumerate((-0.102, 0.102), start=1):
        result.append(
            Aperture(
                name=f"TailFastener_{index:02d}",
                shape_id=SHAPE_IDS["fastener_hole"],
                shape_name="fastener_hole",
                zone_id=ZONE_IDS["moving_tail_pad"],
                cell_index=-1,
                vertices=circle_profile(
                    Vec2(2.785, cross),
                    spec.fastener_hole_radius_m,
                ),
                nominal_bevel=False,
            )
        )
    result.append(
        Aperture(
            name="TailTerminalBore",
            shape_id=SHAPE_IDS["terminal_bore"],
            shape_name="terminal_bore",
            zone_id=ZONE_IDS["moving_terminal"],
            cell_index=-1,
            vertices=circle_profile(
                Vec2(3.105, 0.0),
                spec.terminal_bore_radius_m,
            ),
            nominal_bevel=False,
        )
    )
    return result


def build_fixed_fastener_apertures(spec: HingeDesignSpec) -> list[Aperture]:
    return [
        Aperture(
            name=f"FixedLeafFastener_{index:02d}",
            shape_id=SHAPE_IDS["fastener_hole"],
            shape_name="fastener_hole",
            zone_id=ZONE_IDS["fixed_leaf"],
            cell_index=-1,
            vertices=circle_profile(
                Vec2(centre_x, 0.0),
                spec.fastener_hole_radius_m,
            ),
            nominal_bevel=False,
        )
        for index, centre_x in enumerate((-0.650, -0.315), start=1)
    ]


def build_knuckle_schedule(spec: HingeDesignSpec) -> tuple[KnuckleSegmentPlan, ...]:
    usable = spec.complete_width_m - spec.knuckle_gap_m * (spec.knuckle_count - 1)
    segment_width = usable / spec.knuckle_count
    cursor = -spec.complete_width_m * 0.5
    result = []
    half_seam = spec.knuckle_seam_half_angle_degrees
    for index in range(spec.knuckle_count):
        cross_min = cursor
        cross_max = cross_min + segment_width
        owner_id = (
            OWNER_IDS["moving_leaf"] if index % 2 == 0 else OWNER_IDS["fixed_leaf"]
        )
        result.append(
            KnuckleSegmentPlan(
                name=(
                    f"{'Moving' if owner_id == OWNER_IDS['moving_leaf'] else 'Fixed'}"
                    f"Knuckle_{index + 1:02d}"
                ),
                owner_id=owner_id,
                index=index,
                cross_min_m=cross_min,
                cross_max_m=cross_max,
                axis_x_m=0.0,
                outer_radius_m=spec.knuckle_outer_radius_m,
                inner_radius_m=spec.knuckle_inner_radius_m,
                seam_angle_start_degrees=half_seam if owner_id == 2 else 180.0 + half_seam,
                seam_angle_end_degrees=360.0 - half_seam if owner_id == 2 else 540.0 - half_seam,
            )
        )
        cursor = cross_max + spec.knuckle_gap_m
    return tuple(result)


def point_in_polygon(point: Vec2, polygon: Sequence[Vec2]) -> bool:
    inside = False
    previous = polygon[-1]
    for current in polygon:
        intersects = (current.y > point.y) != (previous.y > point.y)
        if intersects:
            denominator = previous.y - current.y
            if abs(denominator) <= EPSILON:
                previous = current
                continue
            hit_x = (
                (previous.x - current.x)
                * (point.y - current.y)
                / denominator
                + current.x
            )
            if point.x < hit_x:
                inside = not inside
        previous = current
    return inside


def orientation(first: Vec2, second: Vec2, third: Vec2) -> float:
    return (second - first).cross(third - first)


def segments_intersect(
    first_a: Vec2,
    first_b: Vec2,
    second_a: Vec2,
    second_b: Vec2,
) -> bool:
    o1 = orientation(first_a, first_b, second_a)
    o2 = orientation(first_a, first_b, second_b)
    o3 = orientation(second_a, second_b, first_a)
    o4 = orientation(second_a, second_b, first_b)
    return o1 * o2 < -EPSILON and o3 * o4 < -EPSILON


def segment_distance(
    first_a: Vec2,
    first_b: Vec2,
    second_a: Vec2,
    second_b: Vec2,
) -> float:
    if segments_intersect(first_a, first_b, second_a, second_b):
        return 0.0

    def point_segment(point: Vec2, start: Vec2, end: Vec2) -> float:
        route = end - start
        denominator = route.dot(route)
        if denominator <= EPSILON:
            return (point - start).length()
        ratio = min(1.0, max(0.0, (point - start).dot(route) / denominator))
        projection = start + route * ratio
        return (point - projection).length()

    return min(
        point_segment(first_a, second_a, second_b),
        point_segment(first_b, second_a, second_b),
        point_segment(second_a, first_a, first_b),
        point_segment(second_b, first_a, first_b),
    )


def polygon_edges(points: Sequence[Vec2]) -> Iterable[tuple[Vec2, Vec2]]:
    for index, point in enumerate(points):
        yield point, points[(index + 1) % len(points)]


def polygon_distance(first: Sequence[Vec2], second: Sequence[Vec2]) -> float:
    if point_in_polygon(first[0], second) or point_in_polygon(second[0], first):
        return 0.0
    return min(
        segment_distance(first_a, first_b, second_a, second_b)
        for first_a, first_b in polygon_edges(first)
        for second_a, second_b in polygon_edges(second)
    )


def validate_apertures(
    outer: Sequence[ProfileVertex],
    apertures: Sequence[Aperture],
    *,
    minimum_gap_m: float,
) -> float:
    outer_points = [vertex.point for vertex in outer]
    for aperture in apertures:
        if polygon_signed_area(aperture.points) <= 0.0:
            raise ValueError(f"{aperture.name}: aperture is not counter-clockwise")
        for point in aperture.points:
            if not point_in_polygon(point, outer_points):
                raise ValueError(f"{aperture.name}: escaped the plate boundary")

    minimum = math.inf
    for index, first in enumerate(apertures):
        for second in apertures[index + 1 :]:
            distance = polygon_distance(first.points, second.points)
            minimum = min(minimum, distance)
            if distance < minimum_gap_m:
                raise ValueError(
                    f"{first.name} and {second.name} leave only "
                    f"{distance:.6f} m; required {minimum_gap_m:.6f} m"
                )
    return minimum


def count_shapes(apertures: Sequence[Aperture]) -> dict[str, int]:
    counts: dict[str, int] = {}
    for aperture in apertures:
        counts[aperture.shape_name] = counts.get(aperture.shape_name, 0) + 1
    return counts


def build_hinge_plan(
    profile: dict[str, Any] | None = None,
    spec: HingeDesignSpec | None = None,
) -> HingePlan:
    profile = profile or load_profile()
    spec = spec or HingeDesignSpec()
    spec.validate()

    moving_apertures = [
        *build_pattern_apertures(profile, spec),
        *build_moving_fastener_apertures(spec),
    ]
    fixed_apertures = build_fixed_fastener_apertures(spec)
    moving_outer = moving_outer_profile(spec)
    fixed_outer = fixed_outer_profile(spec)
    moving_gap = validate_apertures(
        moving_outer,
        moving_apertures,
        minimum_gap_m=0.025,
    )
    fixed_gap = validate_apertures(
        fixed_outer,
        fixed_apertures,
        minimum_gap_m=0.010,
    )

    moving_plate = PlatePlan(
        name="SM_GH018_MovingLeaf_Openwork",
        owner_id=OWNER_IDS["moving_leaf"],
        outer=moving_outer,
        apertures=tuple(moving_apertures),
        thickness_m=spec.stock_thickness_m,
        outer_bevel_width_m=spec.outer_bevel_width_m,
        outer_bevel_drop_m=spec.outer_bevel_drop_m,
    )
    fixed_plate = PlatePlan(
        name="SM_GH018_FixedLeaf",
        owner_id=OWNER_IDS["fixed_leaf"],
        outer=fixed_outer,
        apertures=tuple(fixed_apertures),
        thickness_m=spec.stock_thickness_m,
        outer_bevel_width_m=spec.outer_bevel_width_m,
        outer_bevel_drop_m=spec.outer_bevel_drop_m,
    )
    shape_counts = count_shapes(moving_apertures)
    required_counts = {
        "S1_central_lancet_aperture": 5,
        "S2_flank_leaf_aperture": 10,
        "S3_junction_petal_aperture": 16,
        "S4_rail_lune_aperture": 8,
        "fastener_hole": 5,
        "terminal_bore": 1,
    }
    if shape_counts != required_counts:
        raise ValueError(
            f"moving-leaf aperture inventory changed: {shape_counts!r}"
        )

    return HingePlan(
        profile_sha256=sha256_file(PROFILE_PATH),
        build_authorized=bool(
            profile["implementation_contract"]["geometry_build_authorized"]
        ),
        spec=spec,
        moving_plate=moving_plate,
        fixed_plate=fixed_plate,
        knuckles=build_knuckle_schedule(spec),
        shape_counts=shape_counts,
        minimum_aperture_gap_m=min(moving_gap, fixed_gap),
    )


def offset_profile(
    vertices: Sequence[ProfileVertex],
    distances: Sequence[float],
    *,
    outward: bool,
    miter_limit: float = 3.0,
) -> tuple[ProfileVertex, ...]:
    """Offset a simple loop with per-vertex distances.

    Aperture loops are counter-clockwise, so their outward normal points right
    of each edge. The plate outer loop is also counter-clockwise; its front
    shoulder uses the inward (left) normals.
    """

    if len(vertices) != len(distances):
        raise ValueError("offset distance count does not match profile")
    points = [vertex.point for vertex in vertices]
    result = []
    for index, vertex in enumerate(vertices):
        previous = points[(index - 1) % len(points)]
        current = vertex.point
        following = points[(index + 1) % len(points)]
        incoming = (current - previous).normalized()
        outgoing = (following - current).normalized()

        def normal(edge: Vec2) -> Vec2:
            right = Vec2(edge.y, -edge.x)
            return right if outward else right * -1.0

        before_normal = normal(incoming)
        after_normal = normal(outgoing)
        bisector_sum = before_normal + after_normal
        if bisector_sum.length() <= EPSILON:
            displacement = before_normal * distances[index]
        else:
            bisector = bisector_sum.normalized()
            denominator = abs(bisector.dot(before_normal))
            raw_miter = distances[index] / max(denominator, 0.25)
            displacement = bisector * min(
                raw_miter,
                distances[index] * miter_limit,
            )
        result.append(
            ProfileVertex(current + displacement, vertex.station_s)
        )
    return ensure_profile_orientation(result, counter_clockwise=True)


def triangulate_loops(
    outer: Sequence[Vec2],
    holes: Sequence[Sequence[Vec2]],
    *,
    vector_type: Any,
    tessellate: Any,
) -> tuple[list[Vec2], list[tuple[int, int, int]]]:
    """Use Blender's native 2D tessellator and recover stable source indices."""

    loops = [
        list(outer),
        *(list(reversed(hole)) for hole in holes),
    ]
    flattened = [point for loop in loops for point in loop]
    lookup = {point.rounded_key(): index for index, point in enumerate(flattened)}
    if len(lookup) != len(flattened):
        raise ValueError("tessellation loops contain duplicate planar vertices")
    polygons = [
        [vector_type((point.x, point.y, 0.0)) for point in loop]
        for loop in loops
    ]
    raw_triangles = tessellate(polygons)
    if not raw_triangles:
        raise ValueError("Blender returned no triangles for the openwork plate")
    triangles = []
    for triangle in raw_triangles:
        if all(isinstance(value, int) for value in triangle):
            indices = tuple(int(value) for value in triangle)
        else:
            indices = tuple(
                lookup[(round(value.x, ROUND_DIGITS), round(value.y, ROUND_DIGITS))]
                for value in triangle
            )
        if len(indices) != 3:
            raise ValueError("Blender tessellator returned a non-triangle")
        triangles.append(indices)
    return flattened, triangles


def blender_point(point: Vec2, depth: float) -> Vec3:
    """Map plan X/cross-Y into Blender X/Z with door-normal depth on Y."""

    return Vec3(point.x, depth, point.y)


def bevel_parameters(
    profile: dict[str, Any],
    spec: HingeDesignSpec,
    aperture: Aperture,
) -> tuple[list[float], list[float], list[float]]:
    widths = []
    shoulder_depths = []
    lip_depths = []
    for vertex in aperture.vertices:
        if aperture.nominal_bevel:
            section = interpolate_section(profile, vertex.station_s)
            local_web_width = (
                spec.nominal_web_width_m * section.width_over_nominal
            )
            widths.append(
                local_web_width * section.bevel_width_over_local_width
            )
            shoulder_depths.append(
                spec.stock_thickness_m * 0.5
                + spec.stock_thickness_m
                * section.crown_rise_over_stock_thickness
            )
            lip_depths.append(
                spec.stock_thickness_m * 0.5
                - spec.stock_thickness_m
                * section.bevel_rise_over_stock_thickness
            )
        else:
            widths.append(0.006)
            shoulder_depths.append(spec.stock_thickness_m * 0.5)
            lip_depths.append(spec.stock_thickness_m * 0.5 - 0.004)
    return widths, shoulder_depths, lip_depths


def append_surface(
    mesh: MeshPlan,
    flattened: Sequence[Vec2],
    triangles: Sequence[tuple[int, int, int]],
    depths: Sequence[float],
    *,
    reverse: bool,
    meta: FaceMeta,
) -> list[int]:
    if len(flattened) != len(depths):
        raise ValueError("surface depth count does not match planar vertices")
    indices = [
        mesh.add_vertex(blender_point(point, depth))
        for point, depth in zip(flattened, depths)
    ]
    for triangle in triangles:
        face = tuple(indices[index] for index in triangle)
        if reverse:
            face = tuple(reversed(face))
        mesh.add_face(face, meta)
    return indices


def add_ring_faces(
    mesh: MeshPlan,
    first: Sequence[int],
    second: Sequence[int],
    *,
    reverse: bool,
    metas: Sequence[FaceMeta],
) -> None:
    if len(first) != len(second) or len(first) != len(metas):
        raise ValueError("ring face schedules do not align")
    count = len(first)
    for index in range(count):
        following = (index + 1) % count
        face = (
            first[index],
            first[following],
            second[following],
            second[index],
        )
        if reverse:
            face = tuple(reversed(face))
        mesh.add_face(face, metas[index])


def compile_plate_mesh(
    profile: dict[str, Any],
    spec: HingeDesignSpec,
    plate: PlatePlan,
    *,
    vector_type: Any,
    tessellate: Any,
) -> MeshPlan:
    """Compile a plate with real holes and two explicit front crease loops."""

    mesh = MeshPlan(plate.name)
    outer = plate.outer
    outer_shoulder = offset_profile(
        outer,
        [plate.outer_bevel_width_m] * len(outer),
        outward=False,
    )

    aperture_shoulders = []
    aperture_crown_rings = []
    crown_depths_by_aperture = []
    lip_depths_by_aperture = []
    for aperture in plate.apertures:
        widths, crown_depths, lip_depths = bevel_parameters(
            profile,
            spec,
            aperture,
        )
        aperture_shoulders.append(
            offset_profile(aperture.vertices, widths, outward=True)
        )
        aperture_crown_rings.append(
            offset_profile(
                aperture.vertices,
                [width * 0.55 for width in widths],
                outward=True,
            )
        )
        crown_depths_by_aperture.append(crown_depths)
        lip_depths_by_aperture.append(lip_depths)

    # Front land: outer shoulder is the outer loop; aperture shoulders are the
    # holes. The broad front land stays planar: local crown and bevel level
    # changes are confined to dedicated concentric rings around each opening
    # instead of leaking through the tessellator as accidental long facets.
    front_flattened, front_triangles = triangulate_loops(
        [vertex.point for vertex in outer_shoulder],
        [
            [vertex.point for vertex in shoulder]
            for shoulder in aperture_shoulders
        ],
        vector_type=vector_type,
        tessellate=tessellate,
    )
    front_depths = [spec.stock_thickness_m * 0.5] * len(outer_shoulder)
    for shoulder in aperture_shoulders:
        # triangulate_loops reverses every hole to clockwise winding.
        front_depths.extend(
            [spec.stock_thickness_m * 0.5] * len(shoulder)
        )
    append_surface(
        mesh,
        front_flattened,
        front_triangles,
        front_depths,
        reverse=True,
        meta=FaceMeta(
            hinge_zone=plate.owner_id,
            front_face=True,
        ),
    )

    # Rear face remains deliberately planar in the first clay proof.
    rear_flattened, rear_triangles = triangulate_loops(
        [vertex.point for vertex in outer],
        [list(aperture.points) for aperture in plate.apertures],
        vector_type=vector_type,
        tessellate=tessellate,
    )
    rear_indices = append_surface(
        mesh,
        rear_flattened,
        rear_triangles,
        [-spec.stock_thickness_m * 0.5] * len(rear_flattened),
        reverse=False,
        meta=FaceMeta(
            hinge_zone=plate.owner_id,
            rear_face=True,
        ),
    )

    # Create dedicated front rings so the line work is physical geometry.
    outer_lip_indices = [
        mesh.add_vertex(
            blender_point(
                vertex.point,
                spec.stock_thickness_m * 0.5 - plate.outer_bevel_drop_m,
            )
        )
        for vertex in outer
    ]
    outer_shoulder_indices = [
        mesh.add_vertex(
            blender_point(vertex.point, spec.stock_thickness_m * 0.5)
        )
        for vertex in outer_shoulder
    ]
    add_ring_faces(
        mesh,
        outer_lip_indices,
        outer_shoulder_indices,
        reverse=False,
        metas=[
            FaceMeta(
                hinge_zone=plate.owner_id,
                bevel_land=1.0,
                front_face=True,
            )
            for _ in outer
        ],
    )

    rear_cursor = len(outer)
    outer_rear_indices = rear_indices[:rear_cursor]
    add_ring_faces(
        mesh,
        outer_rear_indices,
        outer_lip_indices,
        reverse=False,
        metas=[
            FaceMeta(hinge_zone=plate.owner_id)
            for _ in outer
        ],
    )

    for aperture, shoulder, crown_ring, crown_depths, lip_depths in zip(
        plate.apertures,
        aperture_shoulders,
        aperture_crown_rings,
        crown_depths_by_aperture,
        lip_depths_by_aperture,
    ):
        shoulder_indices = [
            mesh.add_vertex(
                blender_point(
                    vertex.point,
                    spec.stock_thickness_m * 0.5,
                )
            )
            for vertex in shoulder
        ]
        crown_indices = [
            mesh.add_vertex(blender_point(vertex.point, depth))
            for vertex, depth in zip(crown_ring, crown_depths)
        ]
        lip_indices = [
            mesh.add_vertex(blender_point(vertex.point, depth))
            for vertex, depth in zip(aperture.vertices, lip_depths)
        ]
        metas = []
        for vertex in aperture.vertices:
            section = interpolate_section(profile, vertex.station_s)
            metas.append(
                FaceMeta(
                    hinge_zone=aperture.zone_id,
                    cell_index=aperture.cell_index,
                    shape_id=aperture.shape_id,
                    web_station=vertex.station_s,
                    bevel_land=section.flat_land_fraction,
                    front_face=True,
                )
            )
        add_ring_faces(
            mesh,
            shoulder_indices,
            crown_indices,
            reverse=True,
            metas=metas,
        )
        add_ring_faces(
            mesh,
            crown_indices,
            lip_indices,
            reverse=True,
            metas=metas,
        )

        rear_count = len(aperture.vertices)
        # The tessellator receives clockwise hole loops. Restore the authored
        # counter-clockwise order before pairing front and rear wall rings.
        aperture_rear_indices = list(
            reversed(
                rear_indices[
                    rear_cursor : rear_cursor + rear_count
                ]
            )
        )
        rear_cursor += rear_count
        add_ring_faces(
            mesh,
            lip_indices,
            aperture_rear_indices,
            reverse=True,
            metas=[
                FaceMeta(
                    hinge_zone=aperture.zone_id,
                    cell_index=aperture.cell_index,
                    shape_id=aperture.shape_id,
                    web_station=vertex.station_s,
                    aperture_wall=True,
                )
                for vertex in aperture.vertices
            ],
        )

    mesh.validate()
    orient_closed_mesh(mesh)
    return mesh


def signed_mesh_volume(mesh: MeshPlan) -> float:
    volume = 0.0
    for face in mesh.faces:
        first = mesh.vertices[face[0]]
        for index in range(1, len(face) - 1):
            second = mesh.vertices[face[index]]
            third = mesh.vertices[face[index + 1]]
            volume += (
                first.x
                * (second.y * third.z - second.z * third.y)
                + first.y
                * (second.z * third.x - second.x * third.z)
                + first.z
                * (second.x * third.y - second.y * third.x)
            ) / 6.0
    return volume


def orient_closed_mesh(mesh: MeshPlan) -> None:
    volume = signed_mesh_volume(mesh)
    if abs(volume) <= EPSILON:
        raise ValueError(f"{mesh.name}: mesh has zero signed volume")
    if volume < 0.0:
        mesh.faces = [tuple(reversed(face)) for face in mesh.faces]


def compile_knuckle_mesh(
    segment: KnuckleSegmentPlan,
    *,
    radial_steps: int = 72,
) -> MeshPlan:
    """Build one solid open-seam rolled-stock barrel segment."""

    if radial_steps < 8:
        raise ValueError("knuckle needs at least eight radial steps")
    mesh = MeshPlan(segment.name)
    angles = [
        math.radians(
            segment.seam_angle_start_degrees
            + (
                segment.seam_angle_end_degrees
                - segment.seam_angle_start_degrees
            )
            * index
            / radial_steps
        )
        for index in range(radial_steps + 1)
    ]
    rings: dict[tuple[int, int], list[int]] = {}
    for cross_index, cross in enumerate(
        (segment.cross_min_m, segment.cross_max_m)
    ):
        for radius_index, radius in enumerate(
            (segment.inner_radius_m, segment.outer_radius_m)
        ):
            rings[(cross_index, radius_index)] = [
                mesh.add_vertex(
                    Vec3(
                        segment.axis_x_m + radius * math.cos(angle),
                        radius * math.sin(angle),
                        cross,
                    )
                )
                for angle in angles
            ]

    zone = (
        ZONE_IDS["moving_knuckle"]
        if segment.owner_id == OWNER_IDS["moving_leaf"]
        else ZONE_IDS["fixed_knuckle"]
    )
    meta = FaceMeta(
        hinge_zone=zone,
        interface_id=1,
    )

    for radial_index in (0, 1):
        first = rings[(0, radial_index)]
        second = rings[(1, radial_index)]
        for index in range(radial_steps):
            face = (
                first[index],
                first[index + 1],
                second[index + 1],
                second[index],
            )
            if radial_index == 0:
                face = tuple(reversed(face))
            mesh.add_face(face, meta)

    for cross_index in (0, 1):
        inner = rings[(cross_index, 0)]
        outer = rings[(cross_index, 1)]
        for index in range(radial_steps):
            face = (
                inner[index],
                outer[index],
                outer[index + 1],
                inner[index + 1],
            )
            if cross_index == 0:
                face = tuple(reversed(face))
            mesh.add_face(face, meta)

    for angle_index in (0, radial_steps):
        mesh.add_face(
            (
                rings[(0, 0)][angle_index],
                rings[(1, 0)][angle_index],
                rings[(1, 1)][angle_index],
                rings[(0, 1)][angle_index],
            ),
            meta,
        )
    mesh.validate()
    orient_closed_mesh(mesh)
    return mesh


def compile_pintle_mesh(
    spec: HingeDesignSpec,
    *,
    radial_steps: int = 72,
) -> MeshPlan:
    """Build the separate cardinal-safe pin inside the common knuckle bore."""

    if radial_steps < 16 or radial_steps % 4 != 0:
        raise ValueError("pintle sampling must include all cardinal angles")
    mesh = MeshPlan("SM_GH018_Pintle")
    half_length = spec.pintle_length_m * 0.5
    rings = []
    for cross in (-half_length, half_length):
        rings.append(
            [
                mesh.add_vertex(
                    Vec3(
                        spec.pintle_radius_m
                        * math.cos(index * math.tau / radial_steps),
                        spec.pintle_radius_m
                        * math.sin(index * math.tau / radial_steps),
                        cross,
                    )
                )
                for index in range(radial_steps)
            ]
        )
    meta = FaceMeta(
        hinge_zone=ZONE_IDS["pintle"],
        interface_id=1,
    )
    lower, upper = rings
    for index in range(radial_steps):
        following = (index + 1) % radial_steps
        mesh.add_face(
            (
                lower[index],
                lower[following],
                upper[following],
                upper[index],
            ),
            meta,
        )
    mesh.add_face(tuple(reversed(lower)), meta)
    mesh.add_face(tuple(upper), meta)
    mesh.validate()
    orient_closed_mesh(mesh)
    return mesh


def edge_use_counts(mesh: MeshPlan) -> dict[tuple[int, int], int]:
    counts: dict[tuple[int, int], int] = {}
    for face in mesh.faces:
        for index, first in enumerate(face):
            second = face[(index + 1) % len(face)]
            edge = tuple(sorted((first, second)))
            counts[edge] = counts.get(edge, 0) + 1
    return counts


def topology_report(mesh: MeshPlan) -> dict[str, Any]:
    counts = edge_use_counts(mesh)
    boundary_edges = [edge for edge, uses in counts.items() if uses == 1]
    non_manifold_edges = [edge for edge, uses in counts.items() if uses != 2]
    return {
        "vertices": len(mesh.vertices),
        "faces": len(mesh.faces),
        "boundary_edges": len(boundary_edges),
        "non_manifold_edges": len(non_manifold_edges),
        "signed_volume_m3": signed_mesh_volume(mesh),
        "boundary_samples": [
            (
                mesh.vertices[first].rounded_tuple(),
                mesh.vertices[second].rounded_tuple(),
            )
            for first, second in boundary_edges[:12]
        ],
    }


def create_blender_mesh_object(
    bpy_module: Any,
    collection: Any,
    mesh_plan: MeshPlan,
    *,
    material: Any,
    owner_id: int,
) -> Any:
    mesh = bpy_module.data.meshes.new(mesh_plan.name + "_Mesh")
    mesh.from_pydata(
        [vertex.rounded_tuple() for vertex in mesh_plan.vertices],
        [],
        mesh_plan.faces,
    )
    mesh.update(calc_edges=True)
    mesh.materials.append(material)
    obj = bpy_module.data.objects.new(mesh_plan.name, mesh)
    collection.objects.link(obj)
    obj["sinc_asset_id"] = "GH-018-Openwork-Strap-Hinge-v1"
    obj["sinc_geometry_owner"] = owner_id
    obj["sinc_reference_object"] = "Met 55.61.58"
    obj["sinc_build_profile_sha256"] = sha256_file(PROFILE_PATH)
    obj["sinc_runtime_candidate"] = True

    int_lanes = {
        "sinc_hinge_zone": [meta.hinge_zone for meta in mesh_plan.face_meta],
        "sinc_cell_index": [meta.cell_index for meta in mesh_plan.face_meta],
        "sinc_shape_id": [meta.shape_id for meta in mesh_plan.face_meta],
        "sinc_interface_id": [meta.interface_id for meta in mesh_plan.face_meta],
    }
    float_lanes = {
        "sinc_web_station": [meta.web_station for meta in mesh_plan.face_meta],
        "sinc_bevel_land": [meta.bevel_land for meta in mesh_plan.face_meta],
    }
    bool_lanes = {
        "sinc_aperture_wall": [
            meta.aperture_wall for meta in mesh_plan.face_meta
        ],
        "sinc_front_face": [meta.front_face for meta in mesh_plan.face_meta],
        "sinc_rear_face": [meta.rear_face for meta in mesh_plan.face_meta],
    }
    for name, values in int_lanes.items():
        attribute = mesh.attributes.new(name=name, type="INT", domain="FACE")
        for datum, value in zip(attribute.data, values):
            datum.value = int(value)
    for name, values in float_lanes.items():
        attribute = mesh.attributes.new(name=name, type="FLOAT", domain="FACE")
        for datum, value in zip(attribute.data, values):
            datum.value = float(value)
    for name, values in bool_lanes.items():
        attribute = mesh.attributes.new(name=name, type="BOOLEAN", domain="FACE")
        for datum, value in zip(attribute.data, values):
            datum.value = bool(value)
    return obj


def make_clay_material(bpy_module: Any) -> Any:
    material = bpy_module.data.materials.new("M_GH018_NeutralClay")
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = (0.34, 0.35, 0.36, 1.0)
    principled.inputs["Roughness"].default_value = 0.72
    principled.inputs["Metallic"].default_value = 0.0
    return material


def clear_scene(bpy_module: Any) -> None:
    for obj in tuple(bpy_module.data.objects):
        bpy_module.data.objects.remove(obj, do_unlink=True)
    for collection in tuple(bpy_module.data.collections):
        bpy_module.data.collections.remove(collection)


def new_collection(bpy_module: Any, name: str) -> Any:
    collection = bpy_module.data.collections.new(name)
    bpy_module.context.scene.collection.children.link(collection)
    return collection


def validate_built_object(obj: Any) -> None:
    if obj.type != "MESH":
        raise ValueError(f"{obj.name}: runtime object is not a mesh")
    if obj.modifiers:
        raise ValueError(f"{obj.name}: live modifiers are not permitted")
    if not obj.data.attributes.get("sinc_hinge_zone"):
        raise ValueError(f"{obj.name}: semantic hinge-zone lane is missing")
    if not obj.data.attributes.get("sinc_front_face"):
        raise ValueError(f"{obj.name}: front-face lane is missing")
    if not obj.data.attributes.get("sinc_rear_face"):
        raise ValueError(f"{obj.name}: rear-face lane is missing")


def mesh_component_count(mesh: Any) -> int:
    adjacency = [set() for _vertex in mesh.vertices]
    for edge in mesh.edges:
        first, second = edge.vertices
        adjacency[first].add(second)
        adjacency[second].add(first)
    remaining = set(range(len(mesh.vertices)))
    components = 0
    while remaining:
        components += 1
        stack = [remaining.pop()]
        while stack:
            current = stack.pop()
            for following in adjacency[current]:
                if following in remaining:
                    remaining.remove(following)
                    stack.append(following)
    return components


def reopened_mesh_report(obj: Any) -> dict[str, Any]:
    import bmesh

    mesh = obj.data
    working = bmesh.new()
    working.from_mesh(mesh)
    working.normal_update()
    boundary_edges = sum(len(edge.link_faces) == 1 for edge in working.edges)
    non_manifold_edges = sum(len(edge.link_faces) != 2 for edge in working.edges)
    signed_volume = working.calc_volume(signed=True)
    working.free()

    components = mesh_component_count(mesh)
    euler_characteristic = (
        len(mesh.vertices) - len(mesh.edges) + len(mesh.polygons)
    )
    genus_total = None
    if non_manifold_edges == 0:
        numerator = 2 * components - euler_characteristic
        if numerator % 2 != 0:
            raise ValueError(f"{obj.name}: non-integral orientable genus")
        genus_total = numerator // 2

    boolean_counts = {}
    for name in ("sinc_aperture_wall", "sinc_front_face", "sinc_rear_face"):
        attribute = mesh.attributes.get(name)
        if attribute is not None:
            boolean_counts[name] = sum(bool(datum.value) for datum in attribute.data)
    return {
        "vertices": len(mesh.vertices),
        "edges": len(mesh.edges),
        "polygons": len(mesh.polygons),
        "components": components,
        "boundary_edges": boundary_edges,
        "non_manifold_edges": non_manifold_edges,
        "signed_volume_m3": signed_volume,
        "euler_characteristic": euler_characteristic,
        "genus_total": genus_total,
        "dimensions_m": [round(value, 6) for value in obj.dimensions],
        "modifiers": [modifier.type for modifier in obj.modifiers],
        "materials": [material.name for material in mesh.materials],
        "attributes": sorted(attribute.name for attribute in mesh.attributes),
        "semantic_face_counts": boolean_counts,
    }


def runtime_world_bounds(objects: Sequence[Any]) -> dict[str, list[float]]:
    points = [
        obj.matrix_world @ vertex.co
        for obj in objects
        for vertex in obj.data.vertices
    ]
    minimum = [
        min(getattr(point, axis) for point in points)
        for axis in ("x", "y", "z")
    ]
    maximum = [
        max(getattr(point, axis) for point in points)
        for axis in ("x", "y", "z")
    ]
    return {
        "minimum": [round(value, 6) for value in minimum],
        "maximum": [round(value, 6) for value in maximum],
        "extent": [
            round(high - low, 6)
            for low, high in zip(minimum, maximum)
        ],
    }


def validate_reopened_blend(
    bpy_module: Any,
    profile: dict[str, Any],
    plan: HingePlan,
    *,
    output_path: Path,
) -> dict[str, Any]:
    """Reopen the saved file and validate stored data, not builder memory."""

    bpy_module.ops.wm.open_mainfile(filepath=str(output_path))
    runtime = bpy_module.data.collections.get("GH018_OPENWORK_RUNTIME")
    datums = bpy_module.data.collections.get("GH018_OPENWORK_DATUMS")
    if runtime is None or datums is None:
        raise ValueError("saved hinge collections did not reopen")
    runtime_objects = sorted(runtime.objects, key=lambda obj: obj.name)
    expected_names = sorted(
        [
            plan.moving_plate.name,
            plan.fixed_plate.name,
            "SM_GH018_Pintle",
            *(segment.name for segment in plan.knuckles),
        ]
    )
    if [obj.name for obj in runtime_objects] != expected_names:
        raise ValueError("saved runtime object inventory changed")
    if sorted(obj.name for obj in datums.objects) != [
        "SOCKET_GH018_PintleAxis",
        "SOCKET_GH018_Tail",
    ]:
        raise ValueError("saved interface datum inventory changed")

    forbidden_scene_objects = [
        obj.name
        for obj in bpy_module.data.objects
        if obj.type in {"CAMERA", "LIGHT"}
    ]
    if forbidden_scene_objects:
        raise ValueError(
            f"geometry-only file contains presentation objects: "
            f"{forbidden_scene_objects}"
        )
    if bpy_module.context.scene.get("sinc_render_gate") != "NOT_AUTHORED":
        raise ValueError("saved scene render gate changed")
    if bpy_module.context.scene.get("sinc_profile_sha256") != plan.profile_sha256:
        raise ValueError("saved scene profile lineage changed")

    reports = {obj.name: reopened_mesh_report(obj) for obj in runtime_objects}
    required_attributes = set(
        profile["implementation_contract"]["required_semantic_attributes"]
    )
    for obj in runtime_objects:
        report = reports[obj.name]
        if report["components"] != 1:
            raise ValueError(f"{obj.name}: saved mesh is disconnected")
        if report["non_manifold_edges"] != 0:
            raise ValueError(f"{obj.name}: saved mesh is non-manifold")
        if report["signed_volume_m3"] <= 0.0:
            raise ValueError(f"{obj.name}: saved mesh volume is not positive")
        if report["modifiers"]:
            raise ValueError(f"{obj.name}: saved live modifier survived")
        if not required_attributes.issubset(report["attributes"]):
            missing = required_attributes - set(report["attributes"])
            raise ValueError(f"{obj.name}: semantic attributes missing: {missing}")

    expected_genus = {
        plan.moving_plate.name: len(plan.moving_plate.apertures),
        plan.fixed_plate.name: len(plan.fixed_plate.apertures),
        **{segment.name: 0 for segment in plan.knuckles},
        "SM_GH018_Pintle": 0,
    }
    actual_genus = {
        name: report["genus_total"] for name, report in reports.items()
    }
    if actual_genus != expected_genus:
        raise ValueError(
            f"saved through-hole topology changed: {actual_genus!r}"
        )

    bounds = runtime_world_bounds(runtime_objects)
    if bounds["minimum"][0] != -plan.spec.fixed_leaf_length_m:
        raise ValueError("saved fixed-leaf tip escaped the measured envelope")
    if bounds["maximum"][0] != plan.spec.moving_leaf_length_m:
        raise ValueError("saved moving-leaf tip escaped the measured envelope")
    if bounds["extent"][0] != plan.spec.complete_length_m:
        raise ValueError("saved complete hinge length changed")
    if bounds["extent"][2] != plan.spec.complete_width_m:
        raise ValueError("saved complete hinge width changed")

    material = bpy_module.data.materials.get("M_GH018_NeutralClay")
    if material is None:
        raise ValueError("neutral geometry material did not reopen")
    prohibited_nodes = {
        "ShaderNodeTexImage",
        "ShaderNodeTexNoise",
        "ShaderNodeTexVoronoi",
    }
    present_nodes = {
        node.bl_idname
        for node in material.node_tree.nodes
    }
    if present_nodes & prohibited_nodes:
        raise ValueError("geometry checkpoint contains texture-generation nodes")

    return {
        "reopen_validated": True,
        "blender_version": bpy_module.app.version_string,
        "runtime_objects": [obj.name for obj in runtime_objects],
        "datum_objects": sorted(obj.name for obj in datums.objects),
        "camera_or_light_objects": forbidden_scene_objects,
        "world_bounds_m": bounds,
        "mesh_reports": reports,
        "expected_genus": expected_genus,
        "proof_media_authored": False,
        "render_authorized": bool(
            profile["implementation_contract"]["proof_render_authorized"]
        ),
        "images_in_file": sorted(image.name for image in bpy_module.data.images),
        "material_nodes": sorted(present_nodes),
    }


def build_blender_file(
    bpy_module: Any,
    profile: dict[str, Any],
    plan: HingePlan,
    *,
    output_path: Path = OUTPUT_PATH,
) -> dict[str, Any]:
    """Build and save geometry only. This function does not create proof media."""

    from mathutils import Vector
    from mathutils.geometry import tessellate_polygon

    clear_scene(bpy_module)
    runtime = new_collection(bpy_module, "GH018_OPENWORK_RUNTIME")
    datums = new_collection(bpy_module, "GH018_OPENWORK_DATUMS")
    material = make_clay_material(bpy_module)

    moving_mesh = compile_plate_mesh(
        profile,
        plan.spec,
        plan.moving_plate,
        vector_type=Vector,
        tessellate=tessellate_polygon,
    )
    fixed_mesh = compile_plate_mesh(
        profile,
        plan.spec,
        plan.fixed_plate,
        vector_type=Vector,
        tessellate=tessellate_polygon,
    )
    mesh_rows = [
        (moving_mesh, OWNER_IDS["moving_leaf"]),
        (fixed_mesh, OWNER_IDS["fixed_leaf"]),
        (compile_pintle_mesh(plan.spec), OWNER_IDS["pintle"]),
        *(
            (compile_knuckle_mesh(segment), segment.owner_id)
            for segment in plan.knuckles
        ),
    ]
    objects = [
        create_blender_mesh_object(
            bpy_module,
            runtime,
            mesh_plan,
            material=material,
            owner_id=owner_id,
        )
        for mesh_plan, owner_id in mesh_rows
    ]

    pivot = bpy_module.data.objects.new("SOCKET_GH018_PintleAxis", None)
    pivot.empty_display_type = "PLAIN_AXES"
    pivot.empty_display_size = 0.11
    pivot["sinc_interface_id"] = 1
    pivot["sinc_axis"] = "LOCAL_Z_CROSS_STOCK"
    datums.objects.link(pivot)

    tail = bpy_module.data.objects.new("SOCKET_GH018_Tail", None)
    tail.location = (plan.spec.moving_leaf_length_m, 0.0, 0.0)
    tail.empty_display_type = "PLAIN_AXES"
    tail.empty_display_size = 0.08
    tail["sinc_interface_id"] = 2
    datums.objects.link(tail)

    for obj in objects:
        validate_built_object(obj)
    built_object_names = [obj.name for obj in objects]
    reports = {
        mesh_plan.name: topology_report(mesh_plan)
        for mesh_plan, _owner_id in mesh_rows
    }
    for name, report in reports.items():
        if report["non_manifold_edges"] != 0:
            raise ValueError(f"{name}: non-manifold build plan: {report}")
        if report["signed_volume_m3"] <= 0.0:
            raise ValueError(f"{name}: non-positive volume")

    scene = bpy_module.context.scene
    scene["sinc_asset_id"] = "GH-018-Openwork-Strap-Hinge-v1"
    scene["sinc_reference_object"] = "Met 55.61.58"
    scene["sinc_profile_sha256"] = plan.profile_sha256
    scene["sinc_complete_length_m"] = plan.spec.complete_length_m
    scene["sinc_complete_width_m"] = plan.spec.complete_width_m
    scene["sinc_render_gate"] = "NOT_AUTHORED"

    output_path.parent.mkdir(parents=True, exist_ok=True)
    bpy_module.ops.wm.save_as_mainfile(filepath=str(output_path))
    reopened = validate_reopened_blend(
        bpy_module,
        profile,
        plan,
        output_path=output_path,
    )
    manifest_path = output_path.with_name(
        output_path.stem + "_manifest.json"
    )
    manifest = {
        "schema": "iggy-openwork-strap-hinge-geometry-manifest/1.0",
        "asset_id": "GH-018-Openwork-Strap-Hinge-v1",
        "reference_object": "Met 55.61.58",
        "source_hashes": {
            "profile_sha256": plan.profile_sha256,
            "builder_sha256": sha256_file(Path(__file__)),
            "blend_sha256": sha256_file(output_path),
        },
        "plan": plan_summary(plan),
        "saved_file_validation": reopened,
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    return {
        "output": str(output_path),
        "manifest": str(manifest_path),
        "objects": built_object_names,
        "topology": reports,
        "saved_file_validation": reopened,
    }


def require_build_authorization(
    profile: dict[str, Any],
    *,
    build_requested: bool,
) -> None:
    if not build_requested:
        return
    contract = profile["implementation_contract"]
    if not contract["geometry_build_authorized"]:
        raise PermissionError(
            "Geometry build is locked by the reviewed profile. "
            "Show this script to the user before changing the lock."
        )


def plan_summary(plan: HingePlan) -> dict[str, Any]:
    moving_bounds = polygon_bounds(
        [vertex.point for vertex in plan.moving_plate.outer]
    )
    fixed_bounds = polygon_bounds(
        [vertex.point for vertex in plan.fixed_plate.outer]
    )
    return {
        "profile_sha256": plan.profile_sha256,
        "catalogued_complete_envelope_m": {
            "length": plan.spec.complete_length_m,
            "width": plan.spec.complete_width_m,
        },
        "component_spans_m": {
            "fixed_leaf": plan.spec.fixed_leaf_length_m,
            "moving_leaf": plan.spec.moving_leaf_length_m,
        },
        "pattern": {
            "cell_count": plan.spec.cell_count,
            "junction_count": plan.spec.cell_count - 1,
            "field_start_m": plan.spec.pattern_start_m,
            "field_end_m": plan.spec.pattern_end_m,
            "cell_pitch_m": plan.spec.cell_pitch_m,
            "shape_counts": plan.shape_counts,
            "minimum_aperture_gap_m": plan.minimum_aperture_gap_m,
        },
        "components": {
            "moving_plate": {
                "bounds": moving_bounds,
                "apertures": len(plan.moving_plate.apertures),
            },
            "fixed_plate": {
                "bounds": fixed_bounds,
                "apertures": len(plan.fixed_plate.apertures),
            },
            "knuckles": {
                "count": len(plan.knuckles),
                "moving_owned": sum(
                    segment.owner_id == OWNER_IDS["moving_leaf"]
                    for segment in plan.knuckles
                ),
                "fixed_owned": sum(
                    segment.owner_id == OWNER_IDS["fixed_leaf"]
                    for segment in plan.knuckles
                ),
                "leaf_connection_overlap_m": (
                    plan.spec.knuckle_connection_overlap_m
                ),
            },
            "pintle": {
                "radius_m": plan.spec.pintle_radius_m,
                "length_m": plan.spec.pintle_length_m,
                "radial_clearance_m": (
                    plan.spec.knuckle_inner_radius_m
                    - plan.spec.pintle_radius_m
                ),
                "owner_id": OWNER_IDS["pintle"],
            },
        },
        "build_authorized": plan.build_authorized,
        "proof_media_authored": False,
    }


def polygon_bounds(points: Sequence[Vec2]) -> dict[str, float]:
    return {
        "min_x": min(point.x for point in points),
        "max_x": max(point.x for point in points),
        "min_y": min(point.y for point in points),
        "max_y": max(point.y for point in points),
    }


def parse_args(arguments: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--plan-only",
        action="store_true",
        help="Validate and print the pure geometry plan without Blender.",
    )
    parser.add_argument(
        "--build",
        action="store_true",
        help="Build the .blend only when the reviewed profile lock is open.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=OUTPUT_PATH,
        help="Explicit .blend destination for an authorized build.",
    )
    return parser.parse_args(arguments)


def blender_arguments(arguments: Sequence[str]) -> Sequence[str]:
    if "--" not in arguments:
        return ()
    return arguments[arguments.index("--") + 1 :]


def main(arguments: Sequence[str] | None = None) -> int:
    arguments = list(sys.argv[1:] if arguments is None else arguments)
    in_blender = "bpy" in sys.modules
    parsed = parse_args(
        blender_arguments([sys.argv[0], *arguments]) if in_blender else arguments
    )
    profile = load_profile()
    require_build_authorization(profile, build_requested=parsed.build)
    plan = build_hinge_plan(profile)

    if parsed.plan_only or not parsed.build:
        print(json.dumps(plan_summary(plan), indent=2, sort_keys=True))
        return 0
    if not in_blender:
        raise RuntimeError("--build requires Blender's Python runtime")

    import bpy

    result = build_blender_file(
        bpy,
        profile,
        plan,
        output_path=parsed.output.resolve(),
    )
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
