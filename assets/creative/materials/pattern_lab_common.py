"""Shared deterministic image-building primitives for authored material patterns."""

from __future__ import annotations

import math
from pathlib import Path
import struct
import zlib

import numpy as np


def smoothstep(edge0: float, edge1: float, value: np.ndarray) -> np.ndarray:
    edge0_array = np.asarray(edge0, dtype=np.float32)
    edge1_array = np.asarray(edge1, dtype=np.float32)
    if np.any(edge1_array <= edge0_array):
        raise ValueError("smoothstep edge1 must be greater than edge0")
    t = np.clip(
        (value - edge0_array) / (edge1_array - edge0_array), 0.0, 1.0
    )
    return t * t * (3.0 - 2.0 * t)


def mix(
    background: np.ndarray,
    foreground: np.ndarray,
    mask: np.ndarray,
) -> np.ndarray:
    expanded = mask[..., np.newaxis] if background.ndim == mask.ndim + 1 else mask
    return background * (1.0 - expanded) + foreground * expanded


def srgb_to_linear(value: np.ndarray) -> np.ndarray:
    value = np.asarray(value, dtype=np.float32)
    return np.where(
        value <= 0.04045,
        value / 12.92,
        ((value + 0.055) / 1.055) ** 2.4,
    ).astype(np.float32)


def linear_to_srgb(value: np.ndarray) -> np.ndarray:
    value = np.clip(np.asarray(value, dtype=np.float32), 0.0, 1.0)
    return np.where(
        value <= 0.0031308,
        value * 12.92,
        1.055 * np.power(value, 1.0 / 2.4) - 0.055,
    ).astype(np.float32)


def periodic_value_noise(
    resolution: int,
    cells: int,
    seed: int,
) -> np.ndarray:
    return periodic_value_noise_rect(
        resolution,
        cells_x=cells,
        cells_y=cells,
        seed=seed,
    )


def periodic_value_noise_rect(
    resolution: int,
    *,
    cells_x: int,
    cells_y: int,
    seed: int,
) -> np.ndarray:
    """Periodic value noise with independently controlled axis frequencies."""
    if resolution <= 0 or cells_x <= 0 or cells_y <= 0:
        raise ValueError("resolution and noise cells must be positive")
    rng = np.random.default_rng(seed)
    grid = rng.random((cells_y, cells_x), dtype=np.float32)
    coordinate_x = (
        np.arange(resolution, dtype=np.float32) * cells_x / resolution
    )
    coordinate_y = (
        np.arange(resolution, dtype=np.float32) * cells_y / resolution
    )
    base_x = np.floor(coordinate_x).astype(np.int32)
    base_y = np.floor(coordinate_y).astype(np.int32)
    next_x = (base_x + 1) % cells_x
    next_y = (base_y + 1) % cells_y
    fraction_x = coordinate_x - np.floor(coordinate_x)
    fraction_y = coordinate_y - np.floor(coordinate_y)
    fraction_x = (
        fraction_x**3
        * (fraction_x * (fraction_x * 6.0 - 15.0) + 10.0)
    )
    fraction_y = (
        fraction_y**3
        * (fraction_y * (fraction_y * 6.0 - 15.0) + 10.0)
    )

    x0 = base_x[np.newaxis, :]
    x1 = next_x[np.newaxis, :]
    y0 = base_y[:, np.newaxis]
    y1 = next_y[:, np.newaxis]
    fx = fraction_x[np.newaxis, :]
    fy = fraction_y[:, np.newaxis]
    a = grid[y0, x0]
    b = grid[y0, x1]
    c = grid[y1, x0]
    d = grid[y1, x1]
    return (
        a * (1.0 - fx) * (1.0 - fy)
        + b * fx * (1.0 - fy)
        + c * (1.0 - fx) * fy
        + d * fx * fy
    ).astype(np.float32)


def periodic_fbm_rect(
    resolution: int,
    cells_x: int,
    cells_y: int,
    seed: int,
    *,
    octaves: int = 4,
    persistence: float = 0.52,
) -> np.ndarray:
    result = np.zeros((resolution, resolution), dtype=np.float32)
    amplitude = 1.0
    total = 0.0
    for octave in range(octaves):
        result += amplitude * periodic_value_noise_rect(
            resolution,
            cells_x=cells_x * (2**octave),
            cells_y=cells_y * (2**octave),
            seed=seed + octave * 977,
        )
        total += amplitude
        amplitude *= persistence
    result /= total
    low = float(result.min())
    high = float(result.max())
    if high > low:
        result = (result - low) / (high - low)
    return result.astype(np.float32)


def periodic_fbm(
    resolution: int,
    cells: int,
    seed: int,
    *,
    octaves: int = 4,
    persistence: float = 0.52,
) -> np.ndarray:
    result = np.zeros((resolution, resolution), dtype=np.float32)
    amplitude = 1.0
    total = 0.0
    frequency = cells
    for octave in range(octaves):
        result += amplitude * periodic_value_noise(
            resolution, frequency, seed + octave * 977
        )
        total += amplitude
        amplitude *= persistence
        frequency *= 2
    result /= total
    low = float(result.min())
    high = float(result.max())
    if high > low:
        result = (result - low) / (high - low)
    return result.astype(np.float32)


def periodic_gaussian_blur(
    image: np.ndarray,
    sigma_px: float,
) -> np.ndarray:
    if image.ndim != 2:
        raise ValueError("periodic Gaussian blur expects one scalar image")
    if sigma_px <= 0.0:
        return image.astype(np.float32, copy=True)
    height, width = image.shape
    frequency_y = np.fft.fftfreq(height)[:, np.newaxis]
    frequency_x = np.fft.rfftfreq(width)[np.newaxis, :]
    kernel = np.exp(
        -2.0
        * math.pi**2
        * sigma_px**2
        * (frequency_x**2 + frequency_y**2)
    )
    transformed = np.fft.rfft2(image)
    return np.fft.irfft2(
        transformed * kernel, s=image.shape
    ).real.astype(np.float32)


def height_to_normal(
    height_m: np.ndarray,
    meters_per_pixel: float,
    *,
    strength: float = 1.0,
) -> np.ndarray:
    gradient_x = (
        np.roll(height_m, -1, axis=1)
        - np.roll(height_m, 1, axis=1)
    ) / (2.0 * meters_per_pixel)
    gradient_y = (
        np.roll(height_m, -1, axis=0)
        - np.roll(height_m, 1, axis=0)
    ) / (2.0 * meters_per_pixel)
    normal = np.stack(
        [
            -gradient_x * strength,
            -gradient_y * strength,
            np.ones_like(height_m),
        ],
        axis=-1,
    )
    normal /= np.linalg.norm(normal, axis=-1, keepdims=True)
    return normal.astype(np.float32)


def height_to_normal_nonperiodic(
    height_m: np.ndarray,
    *,
    meters_per_pixel_x: float,
    meters_per_pixel_y: float,
    strength: float = 1.0,
) -> np.ndarray:
    """Derive a tangent normal without wrapping opposite host boundaries."""
    height = np.asarray(height_m, dtype=np.float32)
    if height.ndim != 2 or min(height.shape) < 2:
        raise ValueError("non-periodic height must be at least a 2x2 field")
    if not np.isfinite(height).all():
        raise ValueError("non-periodic height contains non-finite values")
    for label, value in (
        ("meters_per_pixel_x", meters_per_pixel_x),
        ("meters_per_pixel_y", meters_per_pixel_y),
        ("strength", strength),
    ):
        if not math.isfinite(value) or value <= 0.0:
            raise ValueError(f"{label} must be finite and positive")
    edge_order = 2 if min(height.shape) >= 3 else 1
    gradient_y, gradient_x = np.gradient(
        height,
        float(meters_per_pixel_y),
        float(meters_per_pixel_x),
        edge_order=edge_order,
    )
    normal = np.stack(
        [
            -gradient_x * float(strength),
            -gradient_y * float(strength),
            np.ones_like(height),
        ],
        axis=-1,
    )
    normal /= np.linalg.norm(normal, axis=-1, keepdims=True)
    return normal.astype(np.float32)


def broad_forging_plane_field(
    coordinate_x_m: np.ndarray,
    coordinate_y_m: np.ndarray,
    rails: list[dict[str, object]],
) -> np.ndarray:
    """Interpolate hand-authored open rails into broad worked planes.

    Each rail contains ``y_m`` and ordered ``knots_m`` pairs of longitudinal
    position and signed height. Longitudinal interpolation is piecewise
    linear, so each interval owns one broad slope rather than one closed
    hammer-head silhouette. Adjacent rails then blend across stock width.
    """
    coordinate_x = np.asarray(coordinate_x_m, dtype=np.float64)
    coordinate_y = np.asarray(coordinate_y_m, dtype=np.float64)
    if coordinate_x.shape != coordinate_y.shape or coordinate_x.ndim != 2:
        raise ValueError("forging coordinates must be matching HxW fields")
    if (
        not np.isfinite(coordinate_x).all()
        or not np.isfinite(coordinate_y).all()
    ):
        raise ValueError("forging coordinates contain non-finite values")
    if len(rails) < 2:
        raise ValueError("broad forging planes require at least two rails")

    rail_y = np.asarray(
        [float(rail["y_m"]) for rail in rails],
        dtype=np.float64,
    )
    if not np.isfinite(rail_y).all() or np.any(np.diff(rail_y) <= 0.0):
        raise ValueError("forging rail y positions must strictly increase")
    coordinate_min_y = float(coordinate_y.min())
    coordinate_max_y = float(coordinate_y.max())
    tolerance = 1.0e-8
    if (
        rail_y[0] > coordinate_min_y + tolerance
        or rail_y[-1] < coordinate_max_y - tolerance
    ):
        raise ValueError("forging rails must cover the host width")

    coordinate_min_x = float(coordinate_x.min())
    coordinate_max_x = float(coordinate_x.max())
    rail_fields = []
    flat_x = coordinate_x.reshape(-1)
    for rail_index, rail in enumerate(rails):
        knots = np.asarray(rail["knots_m"], dtype=np.float64)
        if knots.ndim != 2 or knots.shape[1] != 2 or len(knots) < 3:
            raise ValueError(
                f"forging rail {rail_index} needs at least three x/height knots"
            )
        if not np.isfinite(knots).all() or np.any(np.diff(knots[:, 0]) <= 0.0):
            raise ValueError(
                f"forging rail {rail_index} knot positions must increase"
            )
        if (
            knots[0, 0] > coordinate_min_x + tolerance
            or knots[-1, 0] < coordinate_max_x - tolerance
        ):
            raise ValueError(
                f"forging rail {rail_index} does not cover the host length"
            )
        rail_fields.append(
            np.interp(flat_x, knots[:, 0], knots[:, 1]).reshape(
                coordinate_x.shape
            )
        )
    rail_height = np.stack(rail_fields, axis=0)

    result = np.zeros_like(coordinate_x)
    owned = np.zeros_like(coordinate_x, dtype=bool)
    for rail_index in range(len(rails) - 1):
        lower_y = rail_y[rail_index]
        upper_y = rail_y[rail_index + 1]
        if rail_index == len(rails) - 2:
            region = (coordinate_y >= lower_y) & (coordinate_y <= upper_y)
        else:
            region = (coordinate_y >= lower_y) & (coordinate_y < upper_y)
        blend = np.clip(
            (coordinate_y - lower_y) / (upper_y - lower_y),
            0.0,
            1.0,
        )
        interpolated = (
            rail_height[rail_index] * (1.0 - blend)
            + rail_height[rail_index + 1] * blend
        )
        result = np.where(region, interpolated, result)
        owned |= region
    if not owned.all():
        raise ValueError("one or more forging coordinates have no rail owner")
    return result.astype(np.float32)


def continuous_oxide_layer_fields(
    coordinate_x_m: np.ndarray,
    coordinate_y_m: np.ndarray,
    thickness_rails: list[dict[str, object]],
    *,
    longitudinal_modes: list[dict[str, float]],
    minimum_thickness_m: float,
    maximum_thickness_m: float,
) -> dict[str, np.ndarray]:
    """Build intact oxide coverage and optical thickness as separate lanes.

    The thermal field is an open-rail interpolation. Optional longitudinal
    modes provide subordinate compression flow across the stock; their total
    amplitude may not exceed twenty percent of the declared thickness range.
    Thickness never becomes surface relief or conductive exposure here.
    """
    if (
        not math.isfinite(minimum_thickness_m)
        or not math.isfinite(maximum_thickness_m)
        or minimum_thickness_m <= 0.0
        or maximum_thickness_m <= minimum_thickness_m
    ):
        raise ValueError("oxide thickness bounds must be finite and positive")
    thickness_span = maximum_thickness_m - minimum_thickness_m
    for rail_index, rail in enumerate(thickness_rails):
        knots = np.asarray(rail["knots_m"], dtype=np.float64)
        if knots.ndim != 2 or knots.shape[1] != 2:
            raise ValueError(f"oxide rail {rail_index} has invalid knots")
        if np.any(knots[:, 1] < minimum_thickness_m) or np.any(
            knots[:, 1] > maximum_thickness_m
        ):
            raise ValueError(
                f"oxide rail {rail_index} leaves the thickness envelope"
            )

    thermal_thickness = broad_forging_plane_field(
        coordinate_x_m,
        coordinate_y_m,
        thickness_rails,
    ).astype(np.float64)
    coordinate_x = np.asarray(coordinate_x_m, dtype=np.float64)
    longitudinal_span = float(coordinate_x.max() - coordinate_x.min())
    if longitudinal_span <= 0.0:
        raise ValueError("oxide host needs positive longitudinal extent")
    normalized_x = (
        coordinate_x - float(coordinate_x.min())
    ) / longitudinal_span
    compression_delta = np.zeros_like(thermal_thickness)
    total_mode_amplitude = 0.0
    for mode_index, mode in enumerate(longitudinal_modes):
        cycles = float(mode["cycles"])
        phase = float(mode["phase"])
        amplitude = float(mode["amplitude_m"])
        if (
            not math.isfinite(cycles)
            or not math.isfinite(phase)
            or not math.isfinite(amplitude)
            or cycles <= 0.0
            or amplitude < 0.0
        ):
            raise ValueError(
                f"oxide longitudinal mode {mode_index} is invalid"
            )
        compression_delta += np.sin(
            math.tau * (cycles * normalized_x + phase)
        ) * amplitude
        total_mode_amplitude += abs(amplitude)
    if total_mode_amplitude > thickness_span * 0.20 + 1.0e-12:
        raise ValueError(
            "oxide compression flow exceeds twenty percent of its envelope"
        )

    raw_thickness = thermal_thickness + compression_delta
    if (
        float(raw_thickness.min()) < minimum_thickness_m - 1.0e-10
        or float(raw_thickness.max()) > maximum_thickness_m + 1.0e-10
    ):
        raise ValueError(
            "oxide thermal and compression fields exceed their envelope"
        )
    thickness = np.clip(
        raw_thickness,
        minimum_thickness_m,
        maximum_thickness_m,
    ).astype(np.float32)
    thickness_response = (
        (thickness - minimum_thickness_m) / thickness_span
    ).astype(np.float32)
    if total_mode_amplitude > 0.0:
        compression_response = np.clip(
            0.5 + compression_delta / (2.0 * total_mode_amplitude),
            0.0,
            1.0,
        ).astype(np.float32)
    else:
        compression_response = np.full(
            thickness.shape,
            0.5,
            dtype=np.float32,
        )
    coverage = np.ones(thickness.shape, dtype=np.float32)
    zero = np.zeros(thickness.shape, dtype=np.float32)
    return {
        "coverage": coverage,
        "thickness_m": thickness,
        "thickness_response": thickness_response,
        "compression_response": compression_response,
        "metalness": zero.copy(),
        "surface_height_m": zero.copy(),
    }


def compose_surface_layer_responses(
    oxide_response: np.ndarray,
    conductor_response: np.ndarray,
    exposed_conductor_mask: np.ndarray,
) -> dict[str, np.ndarray]:
    """Compose complete oxide and conductor responses by explicit coverage.

    The RGB inputs are already-evaluated material responses, not parameters for
    one interpolated BSDF. ``exposed_conductor_mask`` is the only material
    identity input: zero retains oxide, one reveals conductor, and fractional
    values represent filtered boundary coverage. The returned metalness lane
    is data-output provenance for an engine adapter; the render path must still
    mix the two complete material lobes.
    """
    oxide = np.asarray(oxide_response, dtype=np.float32)
    conductor = np.asarray(conductor_response, dtype=np.float32)
    exposure = np.asarray(exposed_conductor_mask, dtype=np.float32)
    if oxide.ndim != 3 or oxide.shape[-1] != 3:
        raise ValueError("oxide response must be an HxWx3 field")
    if conductor.shape != oxide.shape:
        raise ValueError("conductor response must match oxide as HxWx3")
    if exposure.shape != oxide.shape[:2]:
        raise ValueError(
            "exposed conductor mask must match the HxW response field"
        )
    if (
        not np.isfinite(oxide).all()
        or not np.isfinite(conductor).all()
        or not np.isfinite(exposure).all()
    ):
        raise ValueError("surface layer fields must contain only finite values")
    if np.any(oxide < 0.0) or np.any(conductor < 0.0):
        raise ValueError("surface layer responses cannot be negative")
    if np.any(exposure < 0.0) or np.any(exposure > 1.0):
        raise ValueError(
            "exposed conductor mask must remain inside zero-to-one coverage"
        )

    conductor_weight = exposure.copy()
    oxide_weight = (1.0 - conductor_weight).astype(np.float32)
    combined = (
        oxide * oxide_weight[..., np.newaxis]
        + conductor * conductor_weight[..., np.newaxis]
    ).astype(np.float32)
    return {
        "combined_response": combined,
        "oxide_weight": oxide_weight,
        "conductor_weight": conductor_weight,
        "compiled_metalness": conductor_weight.copy(),
    }


def component_tangent_uv(
    vertices: np.ndarray,
    loop_vertex_indices: np.ndarray,
    polygon_loop_starts: np.ndarray,
    polygon_loop_totals: np.ndarray,
    polygon_normals: np.ndarray,
    *,
    mode: str,
    axis_origin: tuple[float, float, float] | None = None,
) -> np.ndarray:
    """Build metre-scaled per-corner UVs for a construction-owned tangent.

    The returned U coordinate always owns the material tangent:

    - ``longitudinal_leaf`` follows stock length on broad faces and follows
      the local boundary on end, bevel, and aperture-land faces;
    - ``circumferential_barrel`` follows the circumference on cylindrical
      walls and uses a planar frame on bearing/end faces;
    - ``axial_pin`` follows the cylinder axis on walls and uses a planar frame
      on end faces.

    Per-corner output permits a face to choose a stable construction frame
    without forcing that frame to interpolate through an incompatible
    adjacent face.
    """
    valid_modes = {
        "longitudinal_leaf",
        "circumferential_barrel",
        "axial_pin",
    }
    if mode not in valid_modes:
        raise ValueError(
            f"unsupported tangent mode {mode!r}; expected {sorted(valid_modes)}"
        )

    positions = np.asarray(vertices, dtype=np.float64)
    loop_vertices = np.asarray(loop_vertex_indices, dtype=np.int64)
    loop_starts = np.asarray(polygon_loop_starts, dtype=np.int64)
    loop_totals = np.asarray(polygon_loop_totals, dtype=np.int64)
    normals = np.asarray(polygon_normals, dtype=np.float64)
    if positions.ndim != 2 or positions.shape[1] != 3:
        raise ValueError("vertices must be an Nx3 array")
    if loop_vertices.ndim != 1:
        raise ValueError("loop_vertex_indices must be a one-dimensional array")
    if (
        loop_starts.ndim != 1
        or loop_totals.ndim != 1
        or len(loop_starts) != len(loop_totals)
    ):
        raise ValueError("polygon loop starts and totals must be paired arrays")
    if normals.shape != (len(loop_starts), 3):
        raise ValueError("polygon_normals must contain one vector per polygon")
    if (
        not np.isfinite(positions).all()
        or not np.isfinite(normals).all()
    ):
        raise ValueError("tangent inputs must contain only finite values")
    if len(positions) == 0 or len(loop_vertices) == 0:
        raise ValueError("tangent construction requires non-empty geometry")
    if np.any(loop_vertices < 0) or np.any(loop_vertices >= len(positions)):
        raise ValueError("a loop references a vertex outside the mesh")

    if axis_origin is None:
        origin = (positions.min(axis=0) + positions.max(axis=0)) * 0.5
    else:
        origin = np.asarray(axis_origin, dtype=np.float64)
        if origin.shape != (3,) or not np.isfinite(origin).all():
            raise ValueError("axis_origin must contain three finite components")

    radial = positions[:, :2] - origin[:2]
    radial_length = np.linalg.norm(radial, axis=-1)
    nonzero_radius = radial_length[radial_length > 1.0e-8]
    radius_proxy = (
        float(np.median(nonzero_radius))
        if len(nonzero_radius)
        else 1.0
    )
    uv = np.zeros((len(loop_vertices), 2), dtype=np.float64)
    assigned = np.zeros(len(loop_vertices), dtype=bool)

    for polygon_index, (start, total) in enumerate(
        zip(loop_starts, loop_totals, strict=True)
    ):
        if start < 0 or total < 3 or start + total > len(loop_vertices):
            raise ValueError(
                f"polygon {polygon_index} has an invalid loop range"
            )
        loop_slice = slice(int(start), int(start + total))
        points = positions[loop_vertices[loop_slice]]
        normal = normals[polygon_index]
        normal_length = float(np.linalg.norm(normal))
        if normal_length <= 1.0e-8:
            raise ValueError(f"polygon {polygon_index} has a zero normal")
        normal = normal / normal_length

        if mode == "longitudinal_leaf":
            if abs(float(normal[1])) >= 0.65:
                polygon_u = points[:, 0]
                polygon_v = points[:, 2]
            else:
                boundary_normal = np.array(
                    [normal[0], 0.0, normal[2]],
                    dtype=np.float64,
                )
                boundary_length = float(np.linalg.norm(boundary_normal))
                if boundary_length <= 1.0e-8:
                    tangent = np.array([1.0, 0.0, 0.0])
                else:
                    boundary_normal /= boundary_length
                    tangent = np.array(
                        [
                            boundary_normal[2],
                            0.0,
                            -boundary_normal[0],
                        ]
                    )
                polygon_u = points @ tangent
                polygon_v = points[:, 1]
        else:
            centered = points[:, :2] - origin[:2]
            if abs(float(normal[2])) >= 0.75:
                polygon_u = centered[:, 0]
                polygon_v = centered[:, 1]
            else:
                angle = np.unwrap(
                    np.arctan2(centered[:, 1], centered[:, 0])
                )
                arc_distance = angle * radius_proxy
                axial_distance = points[:, 2] - origin[2]
                if mode == "circumferential_barrel":
                    polygon_u = arc_distance
                    polygon_v = axial_distance
                else:
                    polygon_u = axial_distance
                    polygon_v = arc_distance

        uv[loop_slice, 0] = polygon_u
        uv[loop_slice, 1] = polygon_v
        assigned[loop_slice] = True

    if not assigned.all():
        missing = np.flatnonzero(~assigned)
        raise ValueError(
            f"{len(missing)} mesh loops are not owned by any polygon"
        )
    return uv.astype(np.float32)


def render_material_preview(
    base_color_linear: np.ndarray,
    normal: np.ndarray,
    roughness: np.ndarray,
    ao: np.ndarray,
) -> np.ndarray:
    lights = (
        (
            np.array([-0.42, -0.32, 0.85], dtype=np.float32),
            np.array([1.00, 0.91, 0.78], dtype=np.float32),
            2.2,
        ),
        (
            np.array([0.70, 0.18, 0.69], dtype=np.float32),
            np.array([0.62, 0.76, 1.00], dtype=np.float32),
            1.1,
        ),
        (
            np.array([-0.10, 0.84, 0.53], dtype=np.float32),
            np.array([0.72, 0.88, 0.70], dtype=np.float32),
            0.65,
        ),
    )
    return _render_material_lights(
        base_color_linear,
        normal,
        roughness,
        ao,
        lights,
    )


def render_material_single_light(
    base_color_linear: np.ndarray,
    normal: np.ndarray,
    roughness: np.ndarray,
    ao: np.ndarray,
    *,
    direction: tuple[float, float, float],
    intensity: float = 2.0,
    color: tuple[float, float, float] = (1.0, 1.0, 1.0),
) -> np.ndarray:
    """Render one neutral or tinted light without modifying base color."""
    lights = (
        (
            np.asarray(direction, dtype=np.float32),
            np.asarray(color, dtype=np.float32),
            float(intensity),
        ),
    )
    return _render_material_lights(
        base_color_linear,
        normal,
        roughness,
        ao,
        lights,
    )


def render_conductor_preview(
    specular_f0_linear: np.ndarray,
    normal: np.ndarray,
    roughness: np.ndarray,
    ao: np.ndarray,
    *,
    view_direction: tuple[float, float, float] = (0.0, 0.0, 1.0),
    environment_color: tuple[float, float, float] = (0.055, 0.060, 0.070),
    environment_strength: float = 0.72,
) -> np.ndarray:
    """Render a clean conductor with RGB Fresnel and no diffuse lobe.

    ``specular_f0_linear`` is the conductor's normal-incidence reflectance,
    not a photographed base colour. It may be a three-channel constant or an
    HxWx3 field matching ``roughness``.
    """
    lights = (
        (
            np.array([-0.42, -0.32, 0.85], dtype=np.float32),
            np.ones(3, dtype=np.float32),
            2.0,
        ),
        (
            np.array([0.70, 0.18, 0.69], dtype=np.float32),
            np.ones(3, dtype=np.float32),
            0.85,
        ),
        (
            np.array([-0.10, 0.84, 0.53], dtype=np.float32),
            np.ones(3, dtype=np.float32),
            0.40,
        ),
    )
    return _render_conductor_lights(
        specular_f0_linear,
        normal,
        roughness,
        ao,
        lights,
        view_direction=view_direction,
        environment_color=environment_color,
        environment_strength=environment_strength,
    )


def render_conductor_single_light(
    specular_f0_linear: np.ndarray,
    normal: np.ndarray,
    roughness: np.ndarray,
    ao: np.ndarray,
    *,
    direction: tuple[float, float, float],
    intensity: float = 2.0,
    color: tuple[float, float, float] = (1.0, 1.0, 1.0),
    view_direction: tuple[float, float, float] = (0.0, 0.0, 1.0),
    environment_color: tuple[float, float, float] = (0.055, 0.060, 0.070),
    environment_strength: float = 0.72,
) -> np.ndarray:
    """Render a conductor under one controlled diagnostic light."""
    lights = (
        (
            np.asarray(direction, dtype=np.float32),
            np.asarray(color, dtype=np.float32),
            float(intensity),
        ),
    )
    return _render_conductor_lights(
        specular_f0_linear,
        normal,
        roughness,
        ao,
        lights,
        view_direction=view_direction,
        environment_color=environment_color,
        environment_strength=environment_strength,
    )


def _normalize_direction(
    value: np.ndarray | tuple[float, float, float],
    *,
    label: str,
) -> np.ndarray:
    direction = np.asarray(value, dtype=np.float32)
    if direction.shape != (3,):
        raise ValueError(f"{label} must contain exactly three components")
    magnitude = float(np.linalg.norm(direction))
    if magnitude <= 1.0e-8:
        raise ValueError(f"{label} must be non-zero")
    return direction / magnitude


def _broadcast_rgb_field(
    value: np.ndarray | tuple[float, float, float],
    shape: tuple[int, int],
    *,
    label: str,
) -> np.ndarray:
    field = np.asarray(value, dtype=np.float32)
    if field.shape == (3,):
        field = np.broadcast_to(field, (*shape, 3))
    if field.shape != (*shape, 3):
        raise ValueError(
            f"{label} must be RGB or match the HxWx3 material field"
        )
    if not np.isfinite(field).all():
        raise ValueError(f"{label} contains non-finite values")
    return np.clip(field, 0.0, 1.0)


def _validate_preview_fields(
    normal: np.ndarray,
    roughness: np.ndarray,
    ao: np.ndarray,
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    normal_field = np.asarray(normal, dtype=np.float32)
    roughness_field = np.asarray(roughness, dtype=np.float32)
    ao_field = np.asarray(ao, dtype=np.float32)
    if roughness_field.ndim != 2:
        raise ValueError("roughness must be an HxW field")
    expected_normal_shape = (*roughness_field.shape, 3)
    if normal_field.shape != expected_normal_shape:
        raise ValueError("normal must match roughness as an HxWx3 field")
    if ao_field.shape != roughness_field.shape:
        raise ValueError("AO must match the HxW roughness field")
    if (
        not np.isfinite(normal_field).all()
        or not np.isfinite(roughness_field).all()
        or not np.isfinite(ao_field).all()
    ):
        raise ValueError("preview fields must contain only finite values")
    normal_length = np.linalg.norm(normal_field, axis=-1, keepdims=True)
    if np.any(normal_length <= 1.0e-8):
        raise ValueError("normal field contains a zero-length vector")
    return (
        normal_field / normal_length,
        np.clip(roughness_field, 0.02, 1.0),
        np.clip(ao_field, 0.0, 1.0),
    )


def _render_conductor_lights(
    specular_f0_linear: np.ndarray,
    normal: np.ndarray,
    roughness: np.ndarray,
    ao: np.ndarray,
    lights: tuple[tuple[np.ndarray, np.ndarray, float], ...],
    *,
    view_direction: tuple[float, float, float],
    environment_color: tuple[float, float, float],
    environment_strength: float,
) -> np.ndarray:
    normal_field, roughness_field, ao_field = _validate_preview_fields(
        normal,
        roughness,
        ao,
    )
    f0 = _broadcast_rgb_field(
        specular_f0_linear,
        roughness_field.shape,
        label="specular_f0_linear",
    )
    view = _normalize_direction(view_direction, label="view_direction")
    environment = _broadcast_rgb_field(
        environment_color,
        roughness_field.shape,
        label="environment_color",
    )
    if not math.isfinite(environment_strength) or environment_strength < 0.0:
        raise ValueError("environment_strength must be finite and non-negative")

    normal_dot_view = np.clip(
        np.sum(normal_field * view, axis=-1),
        1.0e-4,
        1.0,
    )
    grazing = (1.0 - normal_dot_view)[..., np.newaxis] ** 5
    roughness_limit = np.maximum(
        (1.0 - roughness_field)[..., np.newaxis],
        f0,
    )
    environment_fresnel = f0 + (roughness_limit - f0) * grazing
    ambient = (
        environment
        * environment_fresnel
        * ao_field[..., np.newaxis]
        * float(environment_strength)
    )
    return _render_microfacet_lights(
        diffuse_color_linear=None,
        specular_f0_linear=f0,
        normal=normal_field,
        roughness=roughness_field,
        ambient_linear=ambient,
        lights=lights,
        view=view,
    )


def _render_material_lights(
    base_color_linear: np.ndarray,
    normal: np.ndarray,
    roughness: np.ndarray,
    ao: np.ndarray,
    lights: tuple[tuple[np.ndarray, np.ndarray, float], ...],
) -> np.ndarray:
    normal_field, roughness_field, ao_field = _validate_preview_fields(
        normal,
        roughness,
        ao,
    )
    base_color = _broadcast_rgb_field(
        base_color_linear,
        roughness_field.shape,
        label="base_color_linear",
    )
    ambient = base_color * (0.08 + 0.18 * ao_field[..., np.newaxis])
    return _render_microfacet_lights(
        diffuse_color_linear=base_color,
        specular_f0_linear=np.full(
            (*roughness_field.shape, 3),
            0.04,
            dtype=np.float32,
        ),
        normal=normal_field,
        roughness=roughness_field,
        ambient_linear=ambient,
        lights=lights,
        view=np.array([0.0, 0.0, 1.0], dtype=np.float32),
    )


def _render_microfacet_lights(
    *,
    diffuse_color_linear: np.ndarray | None,
    specular_f0_linear: np.ndarray,
    normal: np.ndarray,
    roughness: np.ndarray,
    ambient_linear: np.ndarray,
    lights: tuple[tuple[np.ndarray, np.ndarray, float], ...],
    view: np.ndarray,
) -> np.ndarray:
    result = np.asarray(ambient_linear, dtype=np.float32).copy()
    normal_dot_view = np.clip(
        np.sum(normal * view, axis=-1),
        1.0e-4,
        1.0,
    )
    alpha = np.maximum(roughness * roughness, 0.025)
    alpha_squared = alpha * alpha
    geometry_k = ((roughness + 1.0) ** 2) / 8.0
    geometry_view = normal_dot_view / (
        normal_dot_view * (1.0 - geometry_k) + geometry_k
    )

    for direction, color, intensity in lights:
        normalized_direction = _normalize_direction(
            direction,
            label="light direction",
        )
        light_color = _normalize_preview_light_color(color)
        halfway = _normalize_direction(
            normalized_direction + view,
            label="light/view halfway vector",
        )
        normal_dot_light = np.clip(
            np.sum(normal * normalized_direction, axis=-1), 0.0, 1.0
        )
        normal_dot_half = np.clip(
            np.sum(normal * halfway, axis=-1), 0.0, 1.0
        )
        view_dot_half = float(np.clip(np.dot(view, halfway), 0.0, 1.0))
        denominator = (
            normal_dot_half * normal_dot_half * (alpha_squared - 1.0) + 1.0
        )
        distribution = alpha_squared / (
            math.pi * denominator * denominator + 1.0e-7
        )
        fresnel = specular_f0_linear + (
            1.0 - specular_f0_linear
        ) * (1.0 - view_dot_half) ** 5
        geometry_light = normal_dot_light / (
            normal_dot_light * (1.0 - geometry_k) + geometry_k
        )
        geometry = geometry_view * geometry_light
        specular = (
            (
                distribution
                * geometry
                / (
                    4.0
                    * normal_dot_view
                    * normal_dot_light
                    + 1.0e-5
                )
            )[..., np.newaxis]
            * fresnel
        )
        if diffuse_color_linear is None:
            contribution = specular
        else:
            diffuse = diffuse_color_linear * (
                (1.0 - fresnel) / math.pi
            )
            contribution = diffuse + specular
        result += (
            contribution
            * normal_dot_light[..., np.newaxis]
            * light_color
            * float(intensity)
        )

    display_linear = 1.0 - np.exp(-np.maximum(result, 0.0) * 1.25)
    return linear_to_srgb(display_linear)


def _normalize_preview_light_color(
    value: np.ndarray | tuple[float, float, float],
) -> np.ndarray:
    color = np.asarray(value, dtype=np.float32)
    if color.shape != (3,) or not np.isfinite(color).all():
        raise ValueError("light color must contain three finite components")
    return np.maximum(color, 0.0)


def write_png_rgb8(path: str | Path, image: np.ndarray) -> None:
    array = np.clip(np.rint(image * 255.0), 0, 255).astype(np.uint8)
    if array.ndim != 3 or array.shape[2] != 3:
        raise ValueError("RGB PNG writer expects HxWx3 data")
    _write_png(Path(path), array, bit_depth=8, color_type=2)


def write_png_gray16(path: str | Path, image: np.ndarray) -> None:
    array = np.clip(np.rint(image * 65535.0), 0, 65535).astype(">u2")
    if array.ndim != 2:
        raise ValueError("grayscale PNG writer expects HxW data")
    _write_png(Path(path), array, bit_depth=16, color_type=0)


def proof_resize(image: np.ndarray, target: int) -> np.ndarray:
    height, width = image.shape[:2]
    if height == target and width == target:
        return image.astype(np.float32, copy=True)
    if height % target == 0 and width % target == 0:
        factor_y = height // target
        factor_x = width // target
        if image.ndim == 2:
            return image.reshape(
                target, factor_y, target, factor_x
            ).mean(axis=(1, 3))
        channels = image.shape[2]
        return image.reshape(
            target, factor_y, target, factor_x, channels
        ).mean(axis=(1, 3))
    sample_y = np.linspace(0, height - 1, target).astype(np.int32)
    sample_x = np.linspace(0, width - 1, target).astype(np.int32)
    return image[np.ix_(sample_y, sample_x)].astype(np.float32)


def gray_rgb(value: np.ndarray) -> np.ndarray:
    return np.repeat(value[..., np.newaxis], 3, axis=-1).astype(np.float32)


def normalized_range(value: np.ndarray) -> np.ndarray:
    low = float(value.min())
    high = float(value.max())
    if high <= low:
        return np.zeros_like(value, dtype=np.float32)
    return ((value - low) / (high - low)).astype(np.float32)


def scalar_tint(
    value: np.ndarray,
    dark: tuple[float, float, float],
    light: tuple[float, float, float],
    *,
    normalize: bool = False,
) -> np.ndarray:
    field = normalized_range(value) if normalize else np.clip(value, 0.0, 1.0)
    return mix(
        np.broadcast_to(np.asarray(dark, dtype=np.float32), (*field.shape, 3)),
        np.broadcast_to(np.asarray(light, dtype=np.float32), (*field.shape, 3)),
        field,
    ).astype(np.float32)


def signed_delta_rgb(value: np.ndarray) -> np.ndarray:
    scale = float(np.percentile(np.abs(value), 99.0))
    if scale <= 1.0e-12:
        return np.full((*value.shape, 3), 0.12, dtype=np.float32)
    normalized = np.clip(value / scale, -1.0, 1.0)
    negative = np.clip(-normalized, 0.0, 1.0)
    positive = np.clip(normalized, 0.0, 1.0)
    neutral = np.full((*value.shape, 3), 0.12, dtype=np.float32)
    negative_color = np.array([0.90, 0.20, 0.16], dtype=np.float32)
    positive_color = np.array([0.20, 0.68, 0.92], dtype=np.float32)
    result = mix(
        neutral,
        np.broadcast_to(negative_color, neutral.shape),
        negative,
    )
    return mix(
        result,
        np.broadcast_to(positive_color, neutral.shape),
        positive,
    ).astype(np.float32)


def palette_panel(
    colors: np.ndarray,
    weights: np.ndarray | None = None,
    *,
    resolution: int = 256,
) -> np.ndarray:
    palette = np.clip(np.asarray(colors, dtype=np.float32), 0.0, 1.0)
    if palette.ndim != 2 or palette.shape[1] != 3 or len(palette) == 0:
        raise ValueError("palette panel expects Nx3 colors")
    if weights is None:
        normalized_weights = np.full(len(palette), 1.0 / len(palette))
    else:
        normalized_weights = np.maximum(
            np.asarray(weights, dtype=np.float64), 0.0
        )
        normalized_weights /= normalized_weights.sum()
    boundaries = np.rint(
        np.cumsum(np.concatenate([[0.0], normalized_weights])) * resolution
    ).astype(np.int32)
    boundaries[-1] = resolution
    panel = np.full((resolution, resolution, 3), 0.04, dtype=np.float32)
    for index, color in enumerate(palette):
        panel[:, boundaries[index] : boundaries[index + 1]] = color
    band_height = max(3, resolution // 32)
    panel[:band_height] *= 1.18
    panel[-band_height:] *= 0.68
    return np.clip(panel, 0.0, 1.0)


def profile_panel(
    profiles: list[np.ndarray],
    colors: list[tuple[float, float, float]],
    *,
    resolution: int = 256,
) -> np.ndarray:
    if len(profiles) != len(colors) or not profiles:
        raise ValueError("profile panel needs matching profiles and colors")
    prepared = [
        np.asarray(profile, dtype=np.float32).reshape(-1) for profile in profiles
    ]
    low = min(float(profile.min()) for profile in prepared)
    high = max(float(profile.max()) for profile in prepared)
    scale = max(high - low, 1.0e-8)
    panel = np.full((resolution, resolution, 3), 0.055, dtype=np.float32)
    for grid in (0.25, 0.50, 0.75):
        position = int(grid * (resolution - 1))
        panel[position - 1 : position + 1] = 0.14
        panel[:, position - 1 : position + 1] = 0.14
    source_x = np.linspace(0, len(prepared[0]) - 1, resolution).astype(np.int32)
    for profile, color in zip(prepared, colors, strict=True):
        sampled = profile[
            np.minimum(source_x, len(profile) - 1)
        ]
        y = np.rint(
            (1.0 - (sampled - low) / scale) * (resolution - 1)
        ).astype(np.int32)
        for thickness in (-1, 0, 1):
            yy = np.clip(y + thickness, 0, resolution - 1)
            panel[yy, np.arange(resolution)] = color
    return panel


def zoom_square(
    image: np.ndarray,
    *,
    center_u: float,
    center_v: float,
    fraction: float = 0.28,
) -> np.ndarray:
    height, width = image.shape[:2]
    size = max(2, int(min(height, width) * fraction))
    center_x = int(np.clip(center_u, 0.0, 1.0) * (width - 1))
    center_y = int(np.clip(center_v, 0.0, 1.0) * (height - 1))
    x0 = min(max(center_x - size // 2, 0), width - size)
    y0 = min(max(center_y - size // 2, 0), height - size)
    return image[y0 : y0 + size, x0 : x0 + size].copy()


def seam_frame(
    image: np.ndarray,
    *,
    thickness: int = 5,
) -> np.ndarray:
    framed = np.asarray(image, dtype=np.float32).copy()
    if framed.ndim == 2:
        framed = gray_rgb(framed)
    color = np.array([0.98, 0.72, 0.24], dtype=np.float32)
    framed[:thickness] = color
    framed[-thickness:] = color
    framed[:, :thickness] = color
    framed[:, -thickness:] = color
    return framed


def compose_material_chapter(
    title: str,
    subtitle: str,
    panels: list[tuple[str, np.ndarray]],
    notes: list[str],
) -> np.ndarray:
    """Compose one detailed eight-panel material-book chapter."""

    if len(panels) != 8:
        raise ValueError("material chapters require exactly eight panels")
    panel_size = 288
    columns = 4
    rows = 2
    margin = 28
    gap = 18
    title_height = 74
    label_height = 26
    footer_height = 170
    width = margin * 2 + columns * panel_size + (columns - 1) * gap
    height = (
        margin * 2
        + title_height
        + rows * (panel_size + label_height)
        + (rows - 1) * gap
        + footer_height
    )
    page = np.full((height, width, 3), 0.042, dtype=np.float32)
    draw_text(
        page,
        margin,
        margin,
        title,
        color=(0.96, 0.78, 0.34),
        scale=3,
    )
    draw_text(
        page,
        margin,
        margin + 31,
        subtitle,
        color=(0.72, 0.74, 0.70),
        scale=2,
    )
    panel_top = margin + title_height
    for index, (label, image) in enumerate(panels):
        row = index // columns
        column = index % columns
        x = margin + column * (panel_size + gap)
        y = panel_top + row * (panel_size + label_height + gap)
        panel = proof_resize(image, panel_size)
        page[y - 2 : y + panel_size + 2, x - 2 : x + panel_size + 2] = 0.14
        page[y : y + panel_size, x : x + panel_size] = panel
        draw_text(
            page,
            x + 2,
            y + panel_size + 6,
            label,
            color=(0.88, 0.88, 0.84),
            scale=2,
        )

    footer_y = panel_top + rows * (panel_size + label_height) + (rows - 1) * gap
    note_y = footer_y + 10
    for note in notes[:4]:
        line_count = draw_text_wrapped(
            page,
            margin,
            note_y,
            note,
            max_characters=92,
            color=(0.70, 0.72, 0.68),
            scale=2,
        )
        note_y += line_count * 19 + 6
    return page


def compose_material_book_index(
    title: str,
    chapters: list[tuple[str, np.ndarray, str]],
) -> np.ndarray:
    """Compose an eight-chapter visual table of contents."""

    if len(chapters) != 8:
        raise ValueError("material books require exactly eight chapters")
    panel_size = 256
    columns = 4
    rows = 2
    margin = 26
    gap = 18
    title_height = 68
    caption_height = 56
    width = margin * 2 + columns * panel_size + (columns - 1) * gap
    height = (
        margin * 2
        + title_height
        + rows * (panel_size + caption_height)
        + (rows - 1) * gap
    )
    page = np.full((height, width, 3), 0.04, dtype=np.float32)
    draw_text(
        page,
        margin,
        margin,
        title,
        color=(0.96, 0.78, 0.34),
        scale=3,
    )
    draw_text(
        page,
        margin,
        margin + 31,
        "EIGHT CHAPTER MATERIAL CONSTRUCTION BOOK",
        color=(0.72, 0.74, 0.70),
        scale=2,
    )
    top = margin + title_height
    for index, (chapter_title, hero, summary) in enumerate(chapters):
        row = index // columns
        column = index % columns
        x = margin + column * (panel_size + gap)
        y = top + row * (panel_size + caption_height + gap)
        page[y - 2 : y + panel_size + 2, x - 2 : x + panel_size + 2] = 0.14
        page[y : y + panel_size, x : x + panel_size] = proof_resize(
            hero, panel_size
        )
        draw_text(
            page,
            x + 2,
            y + panel_size + 5,
            f"{index + 1} {chapter_title}",
            color=(0.90, 0.89, 0.82),
            scale=2,
        )
        draw_text_wrapped(
            page,
            x + 2,
            y + panel_size + 27,
            summary,
            max_characters=38,
            color=(0.60, 0.63, 0.60),
            scale=1,
        )
    return page


def draw_text_wrapped(
    canvas: np.ndarray,
    x: int,
    y: int,
    text: str,
    *,
    max_characters: int,
    color: tuple[float, float, float],
    scale: int,
) -> int:
    words = text.upper().split()
    lines: list[str] = []
    current = ""
    for word in words:
        candidate = word if not current else f"{current} {word}"
        if len(candidate) <= max_characters:
            current = candidate
            continue
        if current:
            lines.append(current)
        current = word
    if current:
        lines.append(current)
    for line_index, line in enumerate(lines):
        draw_text(
            canvas,
            x,
            y + line_index * (8 * scale + 3),
            line,
            color=color,
            scale=scale,
        )
    return len(lines)


_FONT = {
    "A": "01110/10001/10001/11111/10001/10001/10001",
    "B": "11110/10001/10001/11110/10001/10001/11110",
    "C": "01111/10000/10000/10000/10000/10000/01111",
    "D": "11110/10001/10001/10001/10001/10001/11110",
    "E": "11111/10000/10000/11110/10000/10000/11111",
    "F": "11111/10000/10000/11110/10000/10000/10000",
    "G": "01111/10000/10000/10111/10001/10001/01111",
    "H": "10001/10001/10001/11111/10001/10001/10001",
    "I": "11111/00100/00100/00100/00100/00100/11111",
    "J": "00111/00010/00010/00010/10010/10010/01100",
    "K": "10001/10010/10100/11000/10100/10010/10001",
    "L": "10000/10000/10000/10000/10000/10000/11111",
    "M": "10001/11011/10101/10101/10001/10001/10001",
    "N": "10001/11001/10101/10011/10001/10001/10001",
    "O": "01110/10001/10001/10001/10001/10001/01110",
    "P": "11110/10001/10001/11110/10000/10000/10000",
    "Q": "01110/10001/10001/10001/10101/10010/01101",
    "R": "11110/10001/10001/11110/10100/10010/10001",
    "S": "01111/10000/10000/01110/00001/00001/11110",
    "T": "11111/00100/00100/00100/00100/00100/00100",
    "U": "10001/10001/10001/10001/10001/10001/01110",
    "V": "10001/10001/10001/10001/10001/01010/00100",
    "W": "10001/10001/10001/10101/10101/10101/01010",
    "X": "10001/01010/00100/00100/00100/01010/10001",
    "Y": "10001/10001/01010/00100/00100/00100/00100",
    "Z": "11111/00001/00010/00100/01000/10000/11111",
    "0": "01110/10001/10011/10101/11001/10001/01110",
    "1": "00100/01100/00100/00100/00100/00100/01110",
    "2": "01110/10001/00001/00010/00100/01000/11111",
    "3": "11110/00001/00001/01110/00001/00001/11110",
    "4": "00110/01010/10010/11111/00010/00010/00010",
    "5": "11111/10000/10000/11110/00001/00001/11110",
    "6": "01110/10000/10000/11110/10001/10001/01110",
    "7": "11111/00001/00010/00100/01000/01000/01000",
    "8": "01110/10001/10001/01110/10001/10001/01110",
    "9": "01110/10001/10001/01111/00001/00001/01110",
    "-": "00000/00000/00000/11111/00000/00000/00000",
    ".": "00000/00000/00000/00000/00000/00110/00110",
    ":": "00000/00110/00110/00000/00110/00110/00000",
    "/": "00001/00010/00100/01000/10000/00000/00000",
    "%": "11001/11010/00100/01000/10110/00110/00000",
    "+": "00000/00100/00100/11111/00100/00100/00000",
    "=": "00000/11111/00000/11111/00000/00000/00000",
    " ": "00000/00000/00000/00000/00000/00000/00000",
}


def draw_text(
    canvas: np.ndarray,
    x: int,
    y: int,
    text: str,
    *,
    color: tuple[float, float, float],
    scale: int,
) -> None:
    cursor = x
    for character in text.upper():
        glyph = _FONT.get(character, _FONT[" "]).split("/")
        for glyph_y, row in enumerate(glyph):
            for glyph_x, pixel in enumerate(row):
                if pixel != "1":
                    continue
                x0 = cursor + glyph_x * scale
                y0 = y + glyph_y * scale
                canvas[y0 : y0 + scale, x0 : x0 + scale] = color
        cursor += 6 * scale


def _write_png(
    path: Path,
    array: np.ndarray,
    *,
    bit_depth: int,
    color_type: int,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    height, width = array.shape[:2]
    scanlines = b"".join(
        b"\x00" + np.ascontiguousarray(array[row]).tobytes()
        for row in range(height)
    )
    header = struct.pack(
        ">IIBBBBB", width, height, bit_depth, color_type, 0, 0, 0
    )
    payload = (
        b"\x89PNG\r\n\x1a\n"
        + _png_chunk(b"IHDR", header)
        + _png_chunk(b"IDAT", zlib.compress(scanlines, level=9))
        + _png_chunk(b"IEND", b"")
    )
    path.write_bytes(payload)


def _png_chunk(name: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + name
        + payload
        + struct.pack(">I", zlib.crc32(name + payload) & 0xFFFFFFFF)
    )
