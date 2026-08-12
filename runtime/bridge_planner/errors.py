class BridgePlannerError(Exception):
    """Base planner error."""


class InputValidationError(BridgePlannerError):
    """The source JSON cannot be planned safely."""

    def __init__(self, errors):
        self.errors = list(errors)
        super().__init__("Invalid .red.json: " + "; ".join(self.errors))


class ConversionInvariantError(BridgePlannerError):
    """A generated field violates the selected conversion policy."""
