-- called right after processing the $[ichl]file directives,
-- right before processing anything else
-- takes the package object as the parameter
function preprocess_hook(p)
	-- p.code has all the input code from the pkg
end


-- called for every $ifile directive
-- takes a table with a string called 'code' inside, the filename, and any extra arguments
-- passed to $ifile. no return value
function include_file_hook(t, filename, ...)
end

-- called after processing anything that's not code (like '$renaming', comments, etc)
-- and right before parsing the actual code.
-- takes the Package object with all the code on the 'code' key. no return value
function preparse_hook(package)
end


-- called after writing all the output.
-- takes the Package object
function post_output_hook(package)
end