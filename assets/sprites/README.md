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
- 640x256 texture (10 columns x 4 rows)
- 64x64 frame size
- Row 0: front/down
- Row 1: left
- Row 2: diagonal
- Row 3: back/up
- Walk cycle uses frames 0-5

If a file is missing, a procedural placeholder sprite sheet is generated in code.
