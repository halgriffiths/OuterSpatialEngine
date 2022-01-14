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

# Example video

https://user-images.githubusercontent.com/90389170/148947854-6283e273-f780-4d5a-af27-6d9f109007c0.mp4

The recorded run shows the market's price/time chart for 120 AI traders competing at 5 TPS. In reality this action-speed would be slowed down way more (otherwise the market changes way too fast!) but it makes for a more interesting demo.

# Example output
| Qt plot  |  ASCII plot   |
|----------|:-------------:|
| ![qt plot](example_qt_plot.png) |  ![ascii plot](example_ASCII_plot.png) |
