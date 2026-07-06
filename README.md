# Vector Sweeper

Vector Sweeper

    A geometric, directional mutation of the classic Minesweeper game built in C++ using the Raylib framework.

  Gameplay Overview & Features

    Vector Field Tracking: Cells do not display neighbor counts; instead, they project normalized mathematical vectors pointing directly toward the nearest gravitational anomaly.

    Proximity Alert System: Dynamic color-grading shifts vectors from cool green to cautionary orange and warning red as proximity to a singularity core decreases.

    Custom Game Loop: Features a complete custom state machine for real-time Win/Loss criteria overlays and dynamic coordinate grid mapping.
