Optional sprite override directory.

If present, Renderer3D will load these PNGs at runtime:
- grass.png
- water.png
- wall.png
- forest.png
- player_sheet.png
- npc_sheet.png
- mob_sheet.png
- mob_dead.png

Animated sheet format:
- 1408x768 texture (22 columns x 12 rows)
- 64x64 frame size
- Row 0: front/down
- Row 1: left
- Row 2: diagonal
- Row 3: back/up
- Columns 0-3: idle
- Columns 4-9: walk cycle (6 frames)
- Columns 10-15: attack
- Columns 16-19: hurt/hit
- Columns 20-21: death/die

If a file is missing, a procedural placeholder sprite sheet is generated in code.
