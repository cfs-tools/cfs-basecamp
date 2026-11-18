# Command to generate the user guide PDF from the markdown file

eds_dom_reference.pdf: eds-dom-reference.md
	egrep -v '^\\' eds-dom-reference.md | pandoc -f markdown -s -N -V documentclass=report -V geometry:margin=1in -o eds_dom_reference.pdf
