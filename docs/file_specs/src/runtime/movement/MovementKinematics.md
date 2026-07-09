# Movement Kinematics

File:

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementKinematics.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementKinematics.cpp`

Verified at: `69514d40`

## Owns

- Movement travel facts derived from start and final positions.
- Travel direction classification: stationary, contour, uphill, downhill.
- Grade percentage and horizontal/vertical travel measurement.

## Does Not Own

- Movement execution or collision checks.
- Slope walkability policy.
- Player motor state or world mutation.
- App-facing movement presentation.

## Reads

- Start and final `Vec3` positions.
- Core vector finite/distance helpers.

## Writes / Mutates

- Does not mutate runtime state.
- Returns `MovementTravelFacts`.

## Calls Out To / Wires Out To

- `MovementSystem` and traversal execution use this for movement observability.
- Debug/receipt layers consume the resulting facts indirectly through movement/traversal results.

## Called By / Entry Points

- `computeMovementTravelFacts(...)`
- `movementTravelDirectionName(...)`

## Invariants

- Non-finite input returns default zero facts.
- Horizontal distance determines grade denominator; near-zero horizontal movement uses zero grade.
- Small vertical deltas classify as contour; zero movement classifies as stationary.

## Tests / Proof Commands

- `rg -n "movement_kinematics_tests|computeMovementTravelFacts|movementTravelDirectionName" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `movement_kinematics_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/movement/MovementPolicy.*`

## Update When

- Travel fact fields, direction thresholds, grade calculation, or direction names change.

## Do Not Update When

- Only slope bands, collision, or player motor control changes.
