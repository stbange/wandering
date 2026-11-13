## ADDED Requirements

### Requirement: Stable attached movement
The simulation SHALL keep indoor player coordinates attached to the castle
exactly once during translation, turning, jumping and terrain-height changes.

#### Scenario: Prolonged turbo travel
- **WHEN** an indoor player is carried through extended turning travel at 25m/s
- **THEN** local position remains stable without drift and the cabin remains level

#### Scenario: Indoor obstacle
- **WHEN** the player walks into a wall or furnishing
- **THEN** collision prevents penetration without changing the view orientation

### Requirement: Incremental look and context lifecycle
Mouse look SHALL apply event deltas without frame-time scaling or pitch overshoot.
Arrow look SHALL never walk or steer. Focus loss SHALL pause and release capture.

#### Scenario: Clamp reversal
- **WHEN** pitch reaches its clamp and the next mouse event reverses direction
- **THEN** the camera immediately moves away from the clamp

#### Scenario: Resume from pause or restore
- **WHEN** a panel closes, focus returns or a valid save is restored
- **THEN** no stale held input moves the player and usable input can be resumed

### Requirement: Occupied helm
Manual castle motion SHALL require helm occupancy and releasing the helm SHALL
brake the castle. E SHALL use a short-range aimed interaction ray.

#### Scenario: Release while moving
- **WHEN** the player releases the helm during forward turbo
- **THEN** castle movement stops and normal on-foot input resumes

#### Scenario: Unaimed interaction
- **WHEN** E is pressed outside helm reach or while looking away
- **THEN** the player does not acquire helm control
