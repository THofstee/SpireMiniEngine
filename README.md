# SpireMiniEngine

SpireMiniEngine is a mini engine that demonstrates how to use Spire to manange shader library and generate shaders for different platforms.

## How to Run:
- Open "GameEngine.sln" in Visual Studio 2015.
- Build the solution.
- Set GameEngine as start-up project.
- Right click GameEngine project and set the following start-up command:

`
-enginedir "$(SolutionDir)EngineContent" -dir "$(SolutionDir)ExampleGame" -gl
`

- Run.
