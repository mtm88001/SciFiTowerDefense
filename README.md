# SciFiTowerDefense

Sci-fi tower defense prototype built in Unreal Engine 5.8.

## Structure

Gameplay lives in the C++ runtime module `Source/SciFiTowerDefense/TowerDefense/`,
with Blueprint subclasses in `Content/TowerDefense/` supplying assets and defaults.

| Class | Responsibility |
| --- | --- |
| `ATDGameModeBase` | Base health, win/loss state, kill-credit awards |
| `ATDWaveSpawner` | Sequential waves, scaling enemy counts, alive/killed/escaped tracking |
| `ATDEnemyPath` | Spline route the enemies follow |
| `ATDEnemyBase` | Spline movement, health, escape damage, kill reward |
| `ATDTowerBase` | Overlap targeting, turret/barrel tracking, firing cadence, footprint, cost |
| `ATDProjectileBase` | Projectile travel and damage application |
| `ATDTowerPlacementPreview` | Ghost mesh and valid/invalid placement material |
| `ATDPlayerController` | Camera input, free-form tower placement and validation, HUD ownership |
| `ATDPlayerState` | Spendable credit economy |
| `UTDHUDWidget` | Gameplay HUD bindings |

Startup map and default game mode are set in `Config/DefaultEngine.ini`
(`/Game/TowerDefense/Maps/TD_Prototype`).

## Setup after cloning

This repository uses **Git LFS** for binary Unreal assets. Install it before cloning:

```
git lfs install
```

### Excluded content

`Content/ParagonSteel/` is **not tracked** in this repository. It is the free
Paragon: Steel asset pack (~2.2 GB) and is re-downloadable from Fab. The enemy
animation blueprint `ABP_TDSteelEnemy` references it, so a fresh clone needs the
pack re-added to `Content/ParagonSteel/` before that path will resolve. The Epic
mannequin under `Content/Characters/` is tracked and is enough to run the prototype.

## Building

Generate project files from `SciFiTowerDefense.uproject` (right-click →
*Generate Visual Studio project files*), then build the `SciFiTowerDefenseEditor`
target. `Binaries/`, `Intermediate/`, `Saved/` and `DerivedDataCache/` are generated
and intentionally untracked.

## Asset conventions

See `Content/TowerDefense/Art/Enemies/readme_enemyassetpipeline.txt` for the
enemy art pipeline and the gameplay/presentation separation contract.
