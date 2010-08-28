#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "uri.h"
#include "uriparser/Uri.h"

/* returns the number of chars required to store the range as a string, including the nul byte */
static int range_size(const UriTextRangeA *r) {
	return r->first ? 1 + (r->afterLast - r->first) : 0;
}

static const char *copy_range(const UriTextRangeA *r, char **buffer) {
	const int size = range_size(r);
	if (size > 0) {
		const char *s = *buffer;
		memcpy(*buffer, r->first, size - 1);
		(*buffer)[size] = '\0';
		*buffer += size;
		return s;
	}
	return 0;
}

static int parse_int(const char *first, const char *after_last) {
	if (first) {
		const int size = after_last - first + 1;
		char buffer[size];

		memcpy(buffer, first, size - 1);
		buffer[size] = '\0';

		return atoi(buffer);
	}
	return 0;
}

static URI *create_uri(const UriUriA *uu) {
	URI *uri = malloc(sizeof(*uri)
		+ range_size(&uu->scheme)
		+ range_size(&uu->userInfo) + 1	/* userinfo will be split on : */
		+ range_size(&uu->hostText)
		+ range_size(&uu->query)
		+ range_size(&uu->fragment));

	if (uri) {
		char *buffer = (char *) (uri + 1);
		uri->scheme = copy_range(&uu->scheme, &buffer);
		uri->host = copy_range(&uu->hostText, &buffer);
		uri->query = copy_range(&uu->query, &buffer);
		uri->fragment = copy_range(&uu->fragment, &buffer);
		uri->port = parse_int(uu->portText.first, uu->portText.afterLast);
	} else {
		/* work around non-conformant malloc() implementations */
		errno = ENOMEM;
	}

	return uri;
}

URI *uri_parse(const char *uri) {
	UriParserStateA state;
	UriUriA uu;
	URI *rv;

	state.uri = &uu;
	if (URI_SUCCESS == uriParseUriA(&state, uri)) {
		rv = create_uri(&uu);
	} else {
		rv = 0;
	}
	uriFreeUriMembersA(&uu);

	return rv;
}

char *uri_build(const URI *uri) {
	return 0;
}

/* NULL-safe string comparison. a < b if a is NULL and b is not (and vice versa). */
#define COMPARE(a, b)			\
	if (a && b) {			\
		int n = strcmp(a, b);	\
		if (n) return n;	\
	} else {			\
		return a ? 1 : -1;	\
	}

int uri_compare(URI *a, URI *b) {
	COMPARE(a->scheme, b->scheme);
	COMPARE(a->host, b->host);

	if (a->port != b->port) {
		return a->port > b->port ? 1 : -1;
	}

	COMPARE(a->path, b->path);
	COMPARE(a->query, b->query);
	COMPARE(a->fragment, b->fragment);

	COMPARE(a->user, b->user);
	COMPARE(a->pass, b->pass);

	return 0;
}
