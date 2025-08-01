# The following has been generated automatically from src/core/project/qgsprojectservervalidator.h
QgsProjectServerValidator.DuplicatedNames = QgsProjectServerValidator.ValidationError.DuplicatedNames
QgsProjectServerValidator.LayerShortName = QgsProjectServerValidator.ValidationError.LayerShortName
QgsProjectServerValidator.LayerEncoding = QgsProjectServerValidator.ValidationError.LayerEncoding
QgsProjectServerValidator.ProjectShortName = QgsProjectServerValidator.ValidationError.ProjectShortName
QgsProjectServerValidator.ProjectRootNameConflict = QgsProjectServerValidator.ValidationError.ProjectRootNameConflict
QgsProjectServerValidator.OnlyMaptipTrueButEmptyMaptip = QgsProjectServerValidator.ValidationError.OnlyMaptipTrueButEmptyMaptip
try:
    QgsProjectServerValidator.ValidationResult.__attribute_docs__ = {'error': 'Error which occurred during the validation process.', 'identifier': 'Identifier related to the error. It can be a layer/group name.'}
    QgsProjectServerValidator.ValidationResult.__annotations__ = {'error': 'QgsProjectServerValidator.ValidationError', 'identifier': 'object'}
    QgsProjectServerValidator.ValidationResult.__doc__ = """Contains the parameters describing a project validation failure."""
    QgsProjectServerValidator.ValidationResult.__group__ = ['project']
except (NameError, AttributeError):
    pass
try:
    QgsProjectServerValidator.displayValidationError = staticmethod(QgsProjectServerValidator.displayValidationError)
    QgsProjectServerValidator.validate = staticmethod(QgsProjectServerValidator.validate)
    QgsProjectServerValidator.__group__ = ['project']
except (NameError, AttributeError):
    pass
