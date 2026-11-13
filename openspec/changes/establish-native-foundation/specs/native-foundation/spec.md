## ADDED Requirements

### Requirement: External editable content
The executable SHALL load validated Lua definitions and original art from
folders beside the binary. Missing or invalid content SHALL produce useful errors.

#### Scenario: Content edit
- **WHEN** valid tuning or furnishing data is edited before launch
- **THEN** the game uses the changed data without recompiling C

### Requirement: Measured verification
The core unit suite SHALL report measured line coverage of at least 95%.
Platform/render tests and human acceptance SHALL be recorded separately.

#### Scenario: Insufficient coverage
- **WHEN** measured authored core line coverage is below 95%
- **THEN** the coverage gate fails rather than treating tests passing as sufficient

### Requirement: Transactional restore
Save restore SHALL validate version, checksum and state before replacing live
state and SHALL return the player to parked on-foot play with support ready.

#### Scenario: Corrupt save
- **WHEN** a corrupt or incompatible save is loaded
- **THEN** live state is unchanged and a recoverable previous generation is offered

### Requirement: Reproducible packages
The project SHALL provide dependency versions, license notices, build instructions
and a package containing the executable and external content.

#### Scenario: Fresh package launch
- **WHEN** the Windows package launches outside the source tree
- **THEN** it resolves its bundled assets and accepts the real gameplay inputs
