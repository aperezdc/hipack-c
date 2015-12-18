BEGIN {
	OBJ_MAP["f"] = "function";
	OBJ_MAP["m"] = "member";
	OBJ_MAP["M"] = "macro";
	OBJ_MAP["t"] = "type";
	OBJ_MAP["v"] = "var";
	in_doc = 0;
	indent = 0;
}

/^[[:space:]]*\/\*[\*\~][fmMtv]?/ {
	if (!in_doc) {
		in_doc = 1;
	}

	objtype = substr($1, length($1));
	if (objtype in OBJ_MAP) {
		objtype = OBJ_MAP[objtype];
		indent = (index($0, "/") - 1);
		if (indent) {
			indent = indent / 4;
		}
		indent++;
	} else {
		indent = 0;
		objtype = "";
	}

	if (length(objtype)) {
		for (i = 1; i < indent; i++) printf("   ");
		printf(".. c:%s::", objtype);
		for (i = 2; i <= NF; i++) printf(" %s", $(i));
	}
	printf("\n\n");
	next;
}

/^[[:space:]]*\*\// {
	in_doc = 0;
	printf("\n");
	next;
}

/^[[:space:]]*\*[[:space:]]*$/ {
	if (in_doc) print "";
	next;
}

{
	if (in_doc != 0) {
		gsub(/^[[:space:]]*\*[[:space:]]/, "");
		if (indent) {
			for (i = 0; i < indent; i++) printf("   ");
		}
		print;
	}
}
