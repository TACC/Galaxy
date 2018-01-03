#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

static void
SetIntAttr(Document& doc, Value& node, string name, int val)
{
	Value::MemberIterator m = doc["descriptor"].FindMember(name.c_str());
	if (m != doc["descriptor"].MemberEnd())
    doc["descriptor"].RemoveMember(name.c_str());
	
  Value v(kObjectType);
  v.SetInt(val);
  Value n; n.SetString(name.c_str(), doc.GetAllocator());
  node.AddMember(n, v, doc.GetAllocator());
}

void
xxx(Document& doc)
{
	SetIntAttr(doc, doc["descriptor"], "xxx", 1);

	StringBuffer sbuf;
	PrettyWriter<StringBuffer> writer(sbuf);
	doc.Accept(writer);

	cout << "xxx: \n";
	cout << sbuf.GetString() << "\n";
}

void
subr(Document& doc)
{
	SetIntAttr(doc, doc["descriptor"], "subr", 1);

	StringBuffer sbuf;
	PrettyWriter<StringBuffer> writer(sbuf);
	doc.Accept(writer);

	cout << "Subr: \n";
	cout << sbuf.GetString() << "\n";
}

void
subr2(Document& doc)
{
	SetIntAttr(doc, doc["descriptor"], "subr", 2);

	StringBuffer sbuf;
	PrettyWriter<StringBuffer> writer(sbuf);
	doc.Accept(writer);

	cout << "Subr: \n";
	cout << sbuf.GetString() << "\n";
}

int 
main(int a, char **b)
{
	Document doc;
	doc.Parse("{}");

	Value attrs(kObjectType);
	doc.AddMember("attributes", attrs, doc.GetAllocator());

	Value desc(kObjectType);
	doc.AddMember("descriptor", desc, doc.GetAllocator());

	SetIntAttr(doc, doc["descriptor"], "main", 1);

	xxx(doc);

	{
		cout << "Main top:\n";
		StringBuffer sbuf;
		PrettyWriter<StringBuffer> writer(sbuf);
		doc.Accept(writer);

		cout << sbuf.GetString() << "\n";
	}

	subr(doc);

	{
		cout << "Main middle:\n";
		StringBuffer sbuf;
		PrettyWriter<StringBuffer> writer(sbuf);
		doc.Accept(writer);

		cout << sbuf.GetString() << "\n";
	}

	subr2(doc);

	{
		cout << "Main bottom:\n";
		StringBuffer sbuf;
		PrettyWriter<StringBuffer> writer(sbuf);
		doc.Accept(writer);

		cout << sbuf.GetString() << "\n";
	}

}
