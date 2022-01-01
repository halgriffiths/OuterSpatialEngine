# OuterSpatialEngine
This is a C++ port of a C# port of an economy engine called bazaarBot with heavy modifications.
Major changes:
 - Asynchronous multithreaded operation instead of tick-based
 - New AI logic using "desperation" functions to reflect financial position
 - Charging broker fees and sales tax to extract profit rather than a bid-ask spread
 - Message-passing structure (to make a future networking port easier)
 - ASCII graph animations (for future ncurses frontend)

The original repo was based on a paper which had some serious issues, many of which were fixed by Vibr8gKiwi in their C# port.
 - Original repo: https://github.com/larsiusprime/bazaarBot
 - C# port: https://github.com/Vibr8gKiwi/bazaarBot2

# Example output
Simulating 100 traders for 500 ticks:
| Qt plot  |  ASCII plot   |
|----------|:-------------:|
| ![qt plot](example_qt_plot.png) |  ![ascii plot](example_ASCII_plot.png) |
