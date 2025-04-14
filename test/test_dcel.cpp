#include "catch.hpp"

#include <vector>

#include "dcel.h"
#include "unit.h"

typedef Dcel<unit, unit, unit> UnitDcel;

TEST_CASE("creating a DCEL") {
	
	UnitDcel dcel;
	REQUIRE(dcel.vertexCount() == 0);
	REQUIRE(dcel.halfEdgeCount() == 0);
	REQUIRE(dcel.faceCount() == 0);

	SECTION("adding vertices and a half-edge to the DCEL") {
		UnitDcel::Vertex a = dcel.addVertex();
		UnitDcel::Vertex b = dcel.addVertex();
		REQUIRE(dcel.vertexCount() == 2);
		REQUIRE(dcel.halfEdgeCount() == 0);
		REQUIRE(dcel.faceCount() == 0);

		UnitDcel::HalfEdge e = dcel.addEdge(a, b);
		a.setOutgoing(e);
		b.setOutgoing(e.twin());
		e.setNext(e.twin());
		e.twin().setNext(e);

		REQUIRE(dcel.vertexCount() == 2);
		REQUIRE(dcel.halfEdgeCount() == 2);
		REQUIRE(dcel.faceCount() == 0);

		SECTION("addFaces()") {
			dcel.addFaces();

			REQUIRE(dcel.faceCount() == 1);
		}
	}
}

SCENARIO("DCEL operations") {

	GIVEN("a DCEL with two vertices and half-edges between them") {
		UnitDcel dcel;
		UnitDcel::Vertex a = dcel.addVertex();
		UnitDcel::Vertex b = dcel.addVertex();
		UnitDcel::HalfEdge e = dcel.addEdge(a, b);
		UnitDcel::HalfEdge e2 = e.twin();
		a.setOutgoing(e);
		b.setOutgoing(e2);
		e.setNext(e2);
		e2.setNext(e);
		dcel.addFaces();
		UnitDcel::Face f = dcel.face(0);

		THEN("vertex pointers are returned correctly") {
			REQUIRE(a.outgoing() == e);
			REQUIRE(a.incoming() == e2);
			REQUIRE(a.incidentFace() == f);

			REQUIRE(b.outgoing() == e2);
			REQUIRE(b.incoming() == e);
			REQUIRE(b.incidentFace() == f);
		}

		THEN("half-edge pointers are returned correctly") {
			REQUIRE(e.origin() == a);
			REQUIRE(e.destination() == b);
			REQUIRE(e.twin() == e2);
			REQUIRE(e.next() == e2);
			REQUIRE(e.previous() == e2);
			REQUIRE(e.nextIncoming() == e);
			REQUIRE(e.previousIncoming() == e);
			REQUIRE(e.incidentFace() == f);
			REQUIRE(e.oppositeFace() == f);

			REQUIRE(e2.origin() == b);
			REQUIRE(e2.destination() == a);
			REQUIRE(e2.twin() == e);
			REQUIRE(e2.next() == e);
			REQUIRE(e2.previous() == e);
			REQUIRE(e2.nextIncoming() == e2);
			REQUIRE(e2.previousIncoming() == e2);
			REQUIRE(e2.incidentFace() == f);
			REQUIRE(e2.oppositeFace() == f);
		}

		THEN("face pointers are returned correctly") {
			REQUIRE((f.boundary() == e || f.boundary() == e2));
			REQUIRE((f.boundaryVertex() == a || f.boundaryVertex() == b));
		}

		THEN("operator== for vertices works correctly") {
			REQUIRE(a == a);
			REQUIRE(b == b);
			REQUIRE(!(a == b));
			REQUIRE(!(b == a));
			REQUIRE(a != b);
			REQUIRE(b != a);
		}

		THEN("the degree of the vertices is returned correctly") {
			REQUIRE(a.degree() == 1);
			REQUIRE(b.degree() == 1);
		}

		WHEN("calling Vertex::forAllOutgoingEdges()") {
			int count = 0;
			a.forAllOutgoingEdges([&count](UnitDcel::HalfEdge) {
				count++;
			});
			THEN("the function is called for each outgoing vertex") {
				REQUIRE(count == 1);
			}
		}

		WHEN("calling Vertex::forAllIncomingEdges()") {
			int count = 0;
			a.forAllIncomingEdges([&count](UnitDcel::HalfEdge) {
				count++;
			});
			THEN("the function is called for each outgoing vertex") {
				REQUIRE(count == 1);
			}
		}
	}
}

TEST_CASE("isValid()") {

	SECTION("valid DCEL") {
		UnitDcel dcel;
		UnitDcel::Vertex a = dcel.addVertex();
		UnitDcel::Vertex b = dcel.addVertex();
		UnitDcel::HalfEdge e = dcel.addEdge(a, b);
		UnitDcel::HalfEdge e2 = e.twin();
		a.setOutgoing(e);
		b.setOutgoing(e2);
		e.setNext(e2);
		e2.setNext(e);
		dcel.addFaces();

		REQUIRE(dcel.isValid(true));
		REQUIRE(dcel.isValid(false));
	}

	SECTION("valid DCEL, but without faces") {
		UnitDcel dcel;
		UnitDcel::Vertex a = dcel.addVertex();
		UnitDcel::Vertex b = dcel.addVertex();
		UnitDcel::HalfEdge e = dcel.addEdge(a, b);
		UnitDcel::HalfEdge e2 = e.twin();
		a.setOutgoing(e);
		b.setOutgoing(e2);
		e.setNext(e2);
		e2.setNext(e);

		REQUIRE(!dcel.isValid(true));
		REQUIRE(dcel.isValid(false));
	}

	SECTION("invalid DCEL") {
		UnitDcel dcel;
		UnitDcel::Vertex a = dcel.addVertex();
		UnitDcel::Vertex b = dcel.addVertex();
		UnitDcel::HalfEdge e = dcel.addEdge(a, b);
		UnitDcel::HalfEdge e2 = e.twin();
		a.setOutgoing(e);
		b.setOutgoing(e2);
		e.setNext(e2);
		//e2.setNext(e); // forgot to set next pointer...

		REQUIRE(!dcel.isValid(true));
		REQUIRE(!dcel.isValid(false));
	}

	SECTION("invalid DCEL") {
		UnitDcel dcel;
		UnitDcel::Vertex a = dcel.addVertex();
		UnitDcel::Vertex b = dcel.addVertex();
		UnitDcel::HalfEdge e = dcel.addEdge(a, b);
		UnitDcel::HalfEdge e2 = e.twin();
		a.setOutgoing(e);
		b.setOutgoing(e); // incorrect outgoing pointer...
		e.setNext(e2);
		e2.setNext(e);

		REQUIRE(!dcel.isValid(true));
		REQUIRE(!dcel.isValid(false));
	}
}

TEST_CASE("splitting, carving and removing a vertex") {

	// some arbitrary data types to test with
	typedef Dcel<int, std::string, double> MyDcel;

	// add vertices
	MyDcel dcel;
	for (int i = 0; i < 5; i++) {
		MyDcel::Vertex v = dcel.addVertex();
		v.setData(i);
	}

	// add half-edges
	dcel.addHalfEdge(dcel.vertex(0));  // 0
	dcel.addHalfEdge(dcel.vertex(1));  // 1
	dcel.addHalfEdge(dcel.vertex(2));  // 2
	dcel.addHalfEdge(dcel.vertex(1));  // 3
	dcel.addHalfEdge(dcel.vertex(3));  // 4
	dcel.addHalfEdge(dcel.vertex(1));  // 5
	dcel.addHalfEdge(dcel.vertex(4));  // 6
	dcel.addHalfEdge(dcel.vertex(1));  // 7
	dcel.addHalfEdge(dcel.vertex(0));  // 8
	dcel.addHalfEdge(dcel.vertex(2));  // 9
	dcel.addHalfEdge(dcel.vertex(0));  // 10
	dcel.addHalfEdge(dcel.vertex(2));  // 11
	dcel.addHalfEdge(dcel.vertex(3));  // 12
	dcel.addHalfEdge(dcel.vertex(3));  // 13
	dcel.addHalfEdge(dcel.vertex(4));  // 14
	dcel.addHalfEdge(dcel.vertex(4));  // 15
	for (int i = 0; i < 16; i++) {
		dcel.halfEdge(i).setData("half-edge " + std::to_string(i));
	}

	dcel.vertex(0).setOutgoing(dcel.halfEdge(8));
	dcel.vertex(1).setOutgoing(dcel.halfEdge(3));
	dcel.vertex(2).setOutgoing(dcel.halfEdge(11));
	dcel.vertex(3).setOutgoing(dcel.halfEdge(4));
	dcel.vertex(4).setOutgoing(dcel.halfEdge(6));

	dcel.halfEdge(0).setTwin(dcel.halfEdge(15));
	dcel.halfEdge(1).setTwin(dcel.halfEdge(2));
	dcel.halfEdge(3).setTwin(dcel.halfEdge(4));
	dcel.halfEdge(5).setTwin(dcel.halfEdge(6));
	dcel.halfEdge(7).setTwin(dcel.halfEdge(8));
	dcel.halfEdge(9).setTwin(dcel.halfEdge(10));
	dcel.halfEdge(11).setTwin(dcel.halfEdge(12));
	dcel.halfEdge(13).setTwin(dcel.halfEdge(14));

	dcel.halfEdge(0).setNext(dcel.halfEdge(6));
	dcel.halfEdge(1).setNext(dcel.halfEdge(9));
	dcel.halfEdge(2).setNext(dcel.halfEdge(3));
	dcel.halfEdge(3).setNext(dcel.halfEdge(12));
	dcel.halfEdge(4).setNext(dcel.halfEdge(5));
	dcel.halfEdge(5).setNext(dcel.halfEdge(14));
	dcel.halfEdge(6).setNext(dcel.halfEdge(7));
	dcel.halfEdge(7).setNext(dcel.halfEdge(0));
	dcel.halfEdge(8).setNext(dcel.halfEdge(1));
	dcel.halfEdge(9).setNext(dcel.halfEdge(8));
	dcel.halfEdge(10).setNext(dcel.halfEdge(11));
	dcel.halfEdge(11).setNext(dcel.halfEdge(13));
	dcel.halfEdge(12).setNext(dcel.halfEdge(2));
	dcel.halfEdge(13).setNext(dcel.halfEdge(15));
	dcel.halfEdge(14).setNext(dcel.halfEdge(4));
	dcel.halfEdge(15).setNext(dcel.halfEdge(10));

	// add faces
	dcel.addFace(dcel.halfEdge(0));  // 0
	dcel.addFace(dcel.halfEdge(10));  // 1
	dcel.addFace(dcel.halfEdge(14));  // 2
	dcel.addFace(dcel.halfEdge(3));  // 3
	dcel.addFace(dcel.halfEdge(1));  // 4
	for (int i = 0; i < 5; i++) {
		dcel.face(i).setData(i);
	}

	dcel.halfEdge(0).setIncidentFace(dcel.face(0));
	dcel.halfEdge(1).setIncidentFace(dcel.face(4));
	dcel.halfEdge(2).setIncidentFace(dcel.face(3));
	dcel.halfEdge(3).setIncidentFace(dcel.face(3));
	dcel.halfEdge(4).setIncidentFace(dcel.face(2));
	dcel.halfEdge(5).setIncidentFace(dcel.face(2));
	dcel.halfEdge(6).setIncidentFace(dcel.face(0));
	dcel.halfEdge(7).setIncidentFace(dcel.face(0));
	dcel.halfEdge(8).setIncidentFace(dcel.face(4));
	dcel.halfEdge(9).setIncidentFace(dcel.face(4));
	dcel.halfEdge(10).setIncidentFace(dcel.face(1));
	dcel.halfEdge(11).setIncidentFace(dcel.face(1));
	dcel.halfEdge(12).setIncidentFace(dcel.face(3));
	dcel.halfEdge(13).setIncidentFace(dcel.face(1));
	dcel.halfEdge(14).setIncidentFace(dcel.face(2));
	dcel.halfEdge(15).setIncidentFace(dcel.face(1));

	REQUIRE(dcel.isValid(true));
	REQUIRE(dcel.vertexCount() == 5);
	REQUIRE(dcel.halfEdgeCount() == 16);
	REQUIRE(dcel.faceCount() == 5);

	SECTION("split along 4-1-2") {
		MyDcel::Vertex vs = dcel.vertex(1).split(dcel.halfEdge(5),
		                                         dcel.halfEdge(1));
		REQUIRE(dcel.vertexCount() == 6);
		REQUIRE(dcel.halfEdgeCount() == 20);
		REQUIRE(dcel.faceCount() == 6);

		// vertex data copied
		REQUIRE(vs.data() == dcel.vertex(1).data());

		MyDcel::HalfEdge e1 = dcel.halfEdge(5).next();
		MyDcel::HalfEdge e2 = dcel.halfEdge(2).previous();

		REQUIRE(e1.origin() == dcel.vertex(4));
		REQUIRE(e1.destination() == vs);
		REQUIRE(e1.next() == e2);
		REQUIRE(e1.incidentFace() == dcel.face(5));
		REQUIRE(e1.oppositeFace() == dcel.face(2));

		REQUIRE(e2.origin() == vs);
		REQUIRE(e2.destination() == dcel.vertex(2));
		REQUIRE(e2.next() == dcel.halfEdge(2));
		REQUIRE(e2.incidentFace() == dcel.face(5));
		REQUIRE(e2.oppositeFace() == dcel.face(3));

		REQUIRE(dcel.halfEdge(2).origin() == dcel.vertex(2));
		REQUIRE(dcel.halfEdge(2).destination() == dcel.vertex(1));
		REQUIRE(dcel.halfEdge(2).next() == dcel.halfEdge(5));
		REQUIRE(dcel.halfEdge(2).incidentFace() == dcel.face(5));
		REQUIRE(dcel.halfEdge(2).oppositeFace() == dcel.face(4));

		REQUIRE(dcel.halfEdge(5).origin() == dcel.vertex(1));
		REQUIRE(dcel.halfEdge(5).destination() == dcel.vertex(4));
		REQUIRE(dcel.halfEdge(5).next() == e1);
		REQUIRE(dcel.halfEdge(5).incidentFace() == dcel.face(5));
		REQUIRE(dcel.halfEdge(5).oppositeFace() == dcel.face(0));

		REQUIRE(dcel.halfEdge(3).origin() == vs);
		REQUIRE(dcel.halfEdge(3).destination() == dcel.vertex(3));

		REQUIRE(dcel.halfEdge(7).origin() == dcel.vertex(1));
		REQUIRE(dcel.halfEdge(7).destination() == dcel.vertex(0));

		REQUIRE(dcel.isValid(true));
	}

	SECTION("split along (wedge between 3 and 4)-1-2") {
		MyDcel::Vertex vs = dcel.vertex(1).split(dcel.wedge(dcel.halfEdge(5)),
		                                         dcel.halfEdge(1));
		REQUIRE(dcel.vertexCount() == 6);
		REQUIRE(dcel.halfEdgeCount() == 18);
		REQUIRE(dcel.faceCount() == 5);

		// vertex data copied
		REQUIRE(vs.data() == dcel.vertex(1).data());

		MyDcel::HalfEdge e = dcel.halfEdge(4).next();

		REQUIRE(e.origin() == vs);
		REQUIRE(e.destination() == dcel.vertex(2));
		REQUIRE(e.next() == dcel.halfEdge(2));
		REQUIRE(e.previous() == dcel.halfEdge(4));
		REQUIRE(e.twin().next() == dcel.halfEdge(3));
		REQUIRE(e.twin().previous() == dcel.halfEdge(12));
		REQUIRE(e.incidentFace() == dcel.face(2));
		REQUIRE(e.oppositeFace() == dcel.face(3));

		REQUIRE(dcel.halfEdge(2).origin() == dcel.vertex(2));
		REQUIRE(dcel.halfEdge(2).destination() == dcel.vertex(1));
		REQUIRE(dcel.halfEdge(2).next() == dcel.halfEdge(5));
		REQUIRE(dcel.halfEdge(2).previous() == e);
		REQUIRE(dcel.halfEdge(2).incidentFace() == dcel.face(2));
		REQUIRE(dcel.halfEdge(2).oppositeFace() == dcel.face(4));

		REQUIRE(dcel.halfEdge(5).origin() == dcel.vertex(1));
		REQUIRE(dcel.halfEdge(5).destination() == dcel.vertex(4));
		REQUIRE(dcel.halfEdge(5).next() == dcel.halfEdge(14));
		REQUIRE(dcel.halfEdge(5).previous() == dcel.halfEdge(2));
		REQUIRE(dcel.halfEdge(5).incidentFace() == dcel.face(2));
		REQUIRE(dcel.halfEdge(5).oppositeFace() == dcel.face(0));

		REQUIRE(dcel.halfEdge(3).origin() == vs);
		REQUIRE(dcel.halfEdge(3).destination() == dcel.vertex(3));

		REQUIRE(dcel.halfEdge(7).origin() == dcel.vertex(1));
		REQUIRE(dcel.halfEdge(7).destination() == dcel.vertex(0));

		REQUIRE(dcel.isValid(true));
	}

	SECTION("split along (wedge between 3 and 4)-(wedge between 0 and 2)") {

		// make sure faces on both side of the split are the same
		dcel.halfEdge(1).setIncidentFace(dcel.face(2));
		dcel.halfEdge(9).setIncidentFace(dcel.face(2));
		dcel.halfEdge(8).setIncidentFace(dcel.face(2));

		MyDcel::Vertex vs = dcel.vertex(1).split(dcel.wedge(dcel.halfEdge(5)),
		                                         dcel.wedge(dcel.halfEdge(1)));
		REQUIRE(dcel.vertexCount() == 6);
		REQUIRE(dcel.halfEdgeCount() == 16);
		// we cannot actually remove the face, so this stays 5
		REQUIRE(dcel.faceCount() == 5);

		// vertex data copied
		REQUIRE(vs.data() == dcel.vertex(1).data());

		REQUIRE(dcel.halfEdge(8).next() == dcel.halfEdge(5));
		REQUIRE(dcel.halfEdge(4).next() == dcel.halfEdge(1));

		REQUIRE(dcel.halfEdge(8).destination() == dcel.vertex(1));
		REQUIRE(dcel.halfEdge(5).origin() == dcel.vertex(1));
		REQUIRE(dcel.halfEdge(4).destination() == vs);
		REQUIRE(dcel.halfEdge(1).origin() == vs);

		// cannot check faces, as face 4 is going to be invalid anyway
		REQUIRE(dcel.isValid(false));
	}

	SECTION("remove vertex 1 and compact") {
		dcel.vertex(1).remove(dcel.halfEdge(7));

		// reported counts should include removed elements
		REQUIRE(dcel.vertexCount() == 5);
		REQUIRE(dcel.halfEdgeCount() == 16);
		REQUIRE(dcel.faceCount() == 5);

		// removed elements should be removed
		REQUIRE(dcel.vertex(1).isRemoved());
		for (int i = 0; i < 16; i++) {
			REQUIRE(dcel.halfEdge(i).isRemoved() == (i >= 1 && i <= 8));
		}
		REQUIRE(!dcel.face(0).isRemoved());
		REQUIRE(!dcel.face(1).isRemoved());
		REQUIRE(dcel.face(2).isRemoved());
		REQUIRE(dcel.face(3).isRemoved());
		REQUIRE(dcel.face(4).isRemoved());

		REQUIRE(dcel.isValid(true));

		// after compacting, the elements should be really gone
		dcel.compact();

		REQUIRE(dcel.vertexCount() == 4);
		REQUIRE(dcel.halfEdgeCount() == 8);
		REQUIRE(dcel.faceCount() == 2);
	}
}
