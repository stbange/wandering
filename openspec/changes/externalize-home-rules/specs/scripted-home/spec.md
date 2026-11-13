## ADDED Requirements

### Requirement: Script-directed validated activities
Home panels SHALL execute external Lua rules through whitelisted commands.
C SHALL retain authoritative transaction validation.

#### Scenario: Rule edit changes an activity
- **WHEN** the script maps the first workshop action to the twine recipe
- **THEN** that action consumes fiber and produces twine through the C transaction

#### Scenario: Invalid or endless script
- **WHEN** a rule returns an invalid command or exceeds its instruction budget
- **THEN** it fails without changing inventory, crops or preparations

### Requirement: Shared displayed and charged costs
Recipe costs and crop durations SHALL load from validated Lua data.
The HUD SHALL display the same costs that C charges.

#### Scenario: Edited recipe
- **WHEN** a recipe changes from two timber to one
- **THEN** its displayed requirement and charged cost become one on relaunch

### Requirement: Relocatable native distribution
The native package SHALL include libraries, content and license notices,
and SHALL resolve assets independently of the working directory.

#### Scenario: Relocated package game test
- **WHEN** the packaged executable runs outside the source tree
- **THEN** real input, rendering and content-loading checks pass
