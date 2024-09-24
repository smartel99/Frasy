local function IsStage(stage) return Context.info.stage == stage end
return {
    IsStage(Stage.Generation),
    IsStage(Stage.Validation),
    IsStage(Stage.Execution)
}
