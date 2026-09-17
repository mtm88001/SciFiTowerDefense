Tower Defense Enemy Asset Pipeline

Gameplay/visual contract
- ATDEnemyBase owns spline travel, health, death, rewards, targeting identity, and its gameplay capsule.
- CharacterMesh0 is presentation only. Keep its collision disabled and do not use mesh bounds, bones, animation, or root motion to drive gameplay.
- Correct imported art with EnemyVisualRelativeLocation, EnemyVisualRelativeRotation, and EnemyVisualRelativeScale. Keep the gameplay actor at scale 1,1,1.
- Normal locomotion must ignore root motion. MovementSpeedForAnimation reflects actual spline displacement and may drive visual animation state.

Import expectations
- Unreal scale is 1 Unreal Unit = 1 centimeter.
- Preferred interchange format: skeletal FBX, targeting FBX 2020.2 compatibility.
- Each character should include a skeletal mesh, valid skeleton and bone hierarchy, UVs, textures/materials, and valid skin weights.
- Preferred animation coverage: Idle, Walk or Run, Attack if needed later, and Death.
- Gameplay must not depend on exact bone names. Bone or socket requirements belong only to optional presentation features.

Different skeletons and retargeting
- Store owned imported meshes in /Game/TowerDefense/Art/Enemies/Meshes.
- Store owned animation assets in /Game/TowerDefense/Art/Enemies/Animations.
- Store IK Rig assets in /Game/TowerDefense/Art/Enemies/Rigs.
- Store IK Retargeter assets in /Game/TowerDefense/Art/Enemies/Retargeters.
- Create retargeters only after both source and target skeletons exist. The current Epic mannequin proves the pipeline and does not constrain future characters to its skeleton.

Future death animation
- ATDEnemyBase already calls OnDeath before destruction. A later milestone may use that hook to enter a visual death state and add a short controlled destruction delay. Keep rewards and death authority in ATDEnemyBase; do not move them into the Animation Blueprint.