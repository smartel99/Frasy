local function IsStage(stage) return Context.info.stage == stage end

return {
    IsStage(Stage.generation),
    IsStage(Stage.validation),
    IsStage(Stage.execution)
}
