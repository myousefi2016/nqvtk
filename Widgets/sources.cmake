# --------------------------------------------------
# Module Classes

SET( ${ProjectName}_${module}_Classes
    NQVTKWidget
    NQVTKWindow
    RenderableControlWidget
)

# --------------------------------------------------
# UI files

SET( ${ProjectName}_${module}_UI
   ${module}/NQVTKWindow.ui
)

# Tell cmake to uic the .ui files
QT4_WRAP_UI( ${ProjectName}_${module}_UIHeaders 
    ${${ProjectName}_${module}_UI} 
)

# Add include directory for generated files
INCLUDE_DIRECTORIES( 
    ${INCLUDE_DIRECTORIES} 
    ${CMAKE_CURRENT_BINARY_DIR} 
)

# --------------------------------------------------
# Collect sources

SET( ${ProjectName}_${module}_Source )

# Find source files
FOREACH( class ${${ProjectName}_${module}_Classes} )
    collect_class_source_files( 
        ${module} 
        ${class} 
        ${ProjectName}_${module}_Headers 
        ${ProjectName}_${module}_Source 
    )
ENDFOREACH( class )

# Setup moc'ing
QT4_AUTOMOC( ${${ProjectName}_${module}_Source} )

# Add generated files
SET( ${ProjectName}_${module}_GeneratedSource
    ${${ProjectName}_${module}_UIHeaders}
)

# --------------------------------------------------
# Add a source group for the .ui files

SOURCE_GROUP( "${modulename}\\Qt UI Files" 
    REGULAR_EXPRESSION "/${module}/.*\\.ui\$" 
)
