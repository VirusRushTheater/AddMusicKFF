cmake_minimum_required(VERSION 3.16)

project(AMKGUI
	VERSION		1.0.8
	DESCRIPTION	"AddmusicK GUI"
	LANGUAGES	CSharp
)

set(AMKGUI_SOURCES
	Program.cs

	Form1.cs
	Form1.Designer.cs

	ResultForm.cs
	ResultForm.Designer.cs

	Properties/AssemblyInfo.cs
	Properties/Resources.Designer.cs
	Properties/Settings.Designer.cs
)

set(AMKGUI_RESOURCES
	Form1.resx
	ResultForm.resx

	Properties/Resources.resx
)

add_executable(AMKGUI
	${AMKGUI_SOURCES}
	${AMKGUI_RESOURCES}
)