Optional sprite override directory.

If present, Renderer3D will load these PNGs at runtime:
- grass.png
- water.png
- wall.png
- forest.png
- warrior.png
- mage.png
- rogie.png
- monster.png
- mob_dead.png

Animated sheet format:
- Player sheets: 1408x768 texture (10 columns x 4 rows)
- Monster sheet: 1408x736 texture (10 columns x 4 rows)
- Row 0: front/down
- Row 1: left
- Row 2: diagonal
- Row 3: back/up
- Columns 0-3: idle
- Columns 4-9: walk cycle (6 frames)

If a file is missing, a procedural placeholder sprite sheet is generated in code.
