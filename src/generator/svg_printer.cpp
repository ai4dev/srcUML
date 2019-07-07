/** \file
 * \brief Generator for visualizing graphs using the XML-based SVG format.
 *
 * \author Tilo Wiedera
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#include "svg_printer.hpp"

using namespace ogdf;

GraphIO::SVGSettings GraphIO::svgSettings;

GraphIO::SVGSettings::SVGSettings(){
	m_margin = 1;
	m_curviness = 0;
	m_bezierInterpolation = false;
	m_fontSize = 10;
	m_fontColor = "#000000";
	m_fontFamily = "Arial";
	m_width = "";
	m_height = "";
}

bool SvgPrinter::draw(std::ostream &os){
	pugi::xml_document doc;
	pugi::xml_node rootNode = writeHeader(doc);

	if(m_clsAttr) {
		drawClusters(rootNode);
	}

	//drawNodes(rootNode);
	drawEdges(rootNode);
	drawNodes(rootNode);
	
	
	doc.save(os);

	return true;
}

pugi::xml_node SvgPrinter::writeHeader(pugi::xml_document &doc){
	pugi::xml_node rootNode = doc.append_child("svg");
	rootNode.append_attribute("xmlns") = "http://www.w3.org/2000/svg";
	rootNode.append_attribute("xmlns:xlink") = "http://www.w3.org/1999/xlink";
	rootNode.append_attribute("xmlns:ev") = "http://www.w3.org/2001/xml-events";
	rootNode.append_attribute("version") = "1.1";
	rootNode.append_attribute("baseProfile") = "full";
	rootNode.append_attribute("style") = "background: white";

	if(!m_settings.width().empty()) {
		rootNode.append_attribute("width") = m_settings.width().c_str();
	}

	if(!m_settings.height().empty()) {
		rootNode.append_attribute("height") = m_settings.height().c_str();
	}

	DRect box = m_clsAttr ? m_clsAttr->boundingBox() : m_attr.boundingBox();

	double margin = m_settings.margin();
	std::stringstream is;
	is << (box.p1().m_x - margin);
	is << " " << (box.p1().m_y - margin);
	is << " " << (box.width() + 2*margin);
	is << " " << (box.height() + 2*margin);
	rootNode.append_attribute("viewBox") = is.str().c_str();

	pugi::xml_node style_node = rootNode.append_child("style");
	style_node.text() = (".font_style {font: " + std::to_string(m_settings.fontSize()) + "px monospace;}").c_str();
	//std::cerr << std::to_string(m_settings.fontSize()) << std::endl;
	pugi::xml_node back_color = rootNode.append_child("rect");
	back_color.append_attribute("width") = "200%";
	back_color.append_attribute("height") = "200%";
	back_color.append_attribute("fill") = "white";

	return rootNode;
}

void SvgPrinter::writeDashArray(pugi::xml_node xmlNode, StrokeType lineStyle, double lineWidth){
	if(lineStyle != StrokeType::None && lineStyle != StrokeType::Solid) {
		std::stringstream is;

		switch(lineStyle) {
		case StrokeType::Dash:
			is << 4*lineWidth << "," << 2*lineWidth;
			break;
		case StrokeType::Dot:
			is << 1*lineWidth << "," << 2*lineWidth;
			break;
		case StrokeType::Dashdot:
			is << 4*lineWidth << "," << 2*lineWidth << "," << 1*lineWidth << "," << 2*lineWidth;
			break;
		case StrokeType::Dashdotdot:
			is << 4*lineWidth << "," << 2*lineWidth << "," << 1*lineWidth << "," << 2*lineWidth << "," << 1*lineWidth << "," << 2*lineWidth;
			break;
		default:
			// will never happen
			break;
		}

		xmlNode.append_attribute("stroke-dasharray") = is.str().c_str();
	}
}

void SvgPrinter::drawNode(pugi::xml_node xmlNode, node v){
	pugi::xml_node g_node; 
	pugi::xml_node shape;
	pugi::xml_node text_node;
	pugi::xml_node line_node;

	std::stringstream is;

	double x = m_attr.x(v);//center coord
	double y = m_attr.y(v);//center coord
	g_node = xmlNode.append_child("g");
	g_node.append_attribute("class") = "font_style";
	g_node.append_attribute("transform") = ("translate(" + std::to_string(x - m_attr.width(v)/2) + ", " + std::to_string(y - m_attr.height(v)/2) + ")").c_str();

	shape = g_node.append_child("rect");

	if (m_attr.has(GraphAttributes::nodeStyle)) {
		shape.append_attribute("fill") = m_attr.fillColor(v).toString().c_str();
		shape.append_attribute("fill-opacity") = (to_string((double)m_attr.fillColor(v).alpha()/255)).c_str();
		shape.append_attribute("stroke-width") = (to_string(m_attr.strokeWidth(v)) + "px").c_str();

		StrokeType lineStyle = m_attr.has(GraphAttributes::nodeStyle) ? m_attr.strokeType(v) : StrokeType::Solid;

		if(lineStyle == StrokeType::None) {
			shape.append_attribute("stroke") = "none";
		} else {
			shape.append_attribute("stroke") = m_attr.strokeColor(v).toString().c_str();
			writeDashArray(shape, lineStyle, m_attr.strokeWidth(v));
		}
	}

	std::string full_text = m_attr.label(v).c_str();
	//std::cerr << "Full Text: " << full_text << '\n';
	int num_lines = 0;
	int prev = 0;
	int largest_line = 0;
	bool new_line = false;
	bool box_divide = false;

	for(int i = 0; i < full_text.size(); i++){//count number of lines
		//look for my inserted tags
		if(full_text.substr(i, 16) == "<svg_box_divide>"){
			box_divide = true;
		}
		if(full_text.substr(i, 14) == "<svg_new_line>"){
			new_line = true;
		}
		if(new_line || box_divide){
			if(i-prev > largest_line){//track largest line for box size.
				largest_line = i-prev;
			}
			prev = i + 14;
			if(box_divide){
				prev += 2;
			}
		}
		new_line = false;
		box_divide = false;
	}

	prev = 0;

	for(int i = 0; i < full_text.size(); i++){
		//look for my inserted tags
		if(full_text.substr(i, 16) == "<svg_box_divide>"){
			box_divide = true;
		}
		if(full_text.substr(i, 14) == "<svg_new_line>"){
			new_line = true;
		}
		//if I found one of my tags take action
		//if new line, add text element
		//if box divide, add text element and line element 
		if(new_line || box_divide){
			num_lines++;
			text_node = g_node.append_child("text");
			text_node.append_attribute("dy") = (std::to_string(.83 + ((num_lines - 1) * 1.1)) + "em").c_str();
			text_node.append_attribute("dx") = ".17em";
			text_node.append_attribute("text-anchor") = "start";
			text_node.append_attribute("fill") = m_settings.fontColor().c_str();
			text_node.append_attribute("textLength") = (std::to_string((i-prev) * .67) + "em").c_str();
			text_node.append_attribute("lengthAdjust") = "spacingAndGlyphs";
			text_node.text() = full_text.substr(prev, i-prev).c_str();

			prev = i + 14;//move past new line

			if(box_divide){
				line_node = g_node.append_child("line");
				line_node.append_attribute("x1") = "0";
				line_node.append_attribute("y1") = (std::to_string(.83 + ((num_lines - 1) * 1.1) + .34) + "em").c_str();
				line_node.append_attribute("x2") = m_attr.width(v);
				line_node.append_attribute("y2") = (std::to_string(.83 + ((num_lines - 1) * 1.1) + .34) + "em").c_str();
				line_node.append_attribute("stroke") = "black";
				line_node.append_attribute("stroke-width") = "1px";
				prev += 2;//move further for box_divide
			}
		}

		new_line = false;
		box_divide = false;
	}


	shape.append_attribute("width") = m_attr.width(v);//(std::to_string(largest_line * .75) + "em").c_str();
	shape.append_attribute("height") = m_attr.height(v);//(std::to_string(num_lines * 1.3) + "em").c_str();
}

void SvgPrinter::drawCluster(pugi::xml_node xmlNode, cluster c){
	OGDF_ASSERT(m_clsAttr);

	pugi::xml_node cluster;

	xmlNode.append_attribute("class") = "font_style";
	pugi::xml_node text_node = xmlNode.append_child("text");
	//text_node.append_attribute("text-anchor") = "start";
	text_node.text() = m_clsAttr->label(c).c_str();

	if (c == m_clsAttr->constClusterGraph().rootCluster()) {
		cluster = xmlNode;
	} else {
		pugi::xml_node clusterXmlNode = xmlNode.append_child("rect");
		clusterXmlNode.append_attribute("x") = m_clsAttr->x(c);
		clusterXmlNode.append_attribute("y") = m_clsAttr->y(c);
		clusterXmlNode.append_attribute("width") = m_clsAttr->width(c);
		clusterXmlNode.append_attribute("height") = m_clsAttr->height(c);
		clusterXmlNode.append_attribute("fill") = m_clsAttr->fillPattern(c) == FillPattern::None ? "none" : m_clsAttr->fillColor(c).toString().c_str();
		clusterXmlNode.append_attribute("fill-opacity") = (to_string((double)m_clsAttr->fillColor(c).alpha()/255)).c_str();
		clusterXmlNode.append_attribute("stroke") = m_clsAttr->strokeType(c) == StrokeType::None ? "none" : m_clsAttr->strokeColor(c).toString().c_str();
		clusterXmlNode.append_attribute("stroke-width") = (to_string(m_clsAttr->strokeWidth(c)) + "px").c_str();
	}
}

void SvgPrinter::drawNodes(pugi::xml_node xmlNode){
	List<node> nodes;
	m_attr.constGraph().allNodes(nodes);

	if (m_attr.has(GraphAttributes::nodeGraphics | GraphAttributes::threeD)) {
		nodes.quicksort(GenericComparer<node, double>([&](node v) { return m_attr.z(v); }));
	}

	for(node v : nodes) {
		drawNode(xmlNode, v);
	}
}

void SvgPrinter::drawClusters(pugi::xml_node xmlNode){
	OGDF_ASSERT(m_clsAttr);

	Queue<cluster> queue;
	queue.append(m_clsAttr->constClusterGraph().rootCluster());

	while(!queue.empty()) {
		cluster c = queue.pop();
		drawCluster(xmlNode.append_child("g"), c);

		for(cluster child : c->children) {
			queue.append(child);
		}
	}
}

void SvgPrinter::drawEdges(pugi::xml_node xmlNode){
	if (m_attr.has(GraphAttributes::edgeGraphics)) {
		xmlNode = xmlNode.append_child("g");

		for(edge e : m_attr.constGraph().edges) {
			drawEdge(xmlNode, e);
		}
	}
}

void SvgPrinter::appendLineStyle(pugi::xml_node line, edge e) {

	StrokeType lineStyle = m_attr.has(GraphAttributes::edgeStyle) ? m_attr.strokeType(e) : StrokeType::Solid;

	if(lineStyle != StrokeType::None) {
		if (m_attr.has(GraphAttributes::edgeStyle)) {
			line.append_attribute("stroke") = m_attr.strokeColor(e).toString().c_str();
			line.append_attribute("stroke-width") = (to_string(m_attr.strokeWidth(e)) + "px").c_str();
			writeDashArray(line, lineStyle, m_attr.strokeWidth(e));
		} else {
			line.append_attribute("stroke") = "#000000";
		}
	}
}

pugi::xml_node SvgPrinter::drawPolygon(pugi::xml_node xmlNode, const std::list<double> points) {
	pugi::xml_node result = xmlNode.append_child("polygon");
	OGDF_ASSERT(points.size() % 2 == 0);

	std::stringstream is;
	bool writeSpace = false;

	for(double p : points) {
		is << p << (writeSpace ? " " : ",");
	}

	result.append_attribute("points") = is.str().c_str();

	return result;
}

double SvgPrinter::getArrowSize(edge e, node v) {
	double result = 0;

	if(m_attr.has(GraphAttributes::edgeArrow) || m_attr.directed()) {
		const double minSize = 3;//(m_attr.has(GraphAttributes::edgeStyle) ? m_attr.strokeWidth(e) : 1) * 3;
		node w = e->opposite(v);
		result = std::max(minSize, (m_attr.width(v) + m_attr.height(v) + m_attr.width(w) + m_attr.height(w)) / 16.0);
	}

	//return result;
	return 20.0;
}

//determines if the point is covered by node v
bool SvgPrinter::isCoveredBy(const DPoint &point, edge e, node v) {
	double arrowSize = getArrowSize(e, v);

	return point.m_x >= m_attr.x(v) - m_attr.width(v)/2 - arrowSize
	    && point.m_x <= m_attr.x(v) + m_attr.width(v)/2 + arrowSize
	    && point.m_y >= m_attr.y(v) - m_attr.height(v)/2 - arrowSize
	    && point.m_y <= m_attr.y(v) + m_attr.height(v)/2 + arrowSize;
}

void SvgPrinter::drawEdge(pugi::xml_node xmlNode, edge e) {
	// draw arrows if G is directed or if arrow types are defined for the edge
	bool drawSourceArrow = false;
	bool drawTargetArrow = false;
	static int edge_count = 0;
	++edge_count;

	if (m_attr.has(GraphAttributes::edgeArrow)) {
		switch (m_attr.arrowType(e)) {
		case EdgeArrow::Undefined:
			drawTargetArrow = m_attr.directed();
			break;
		case EdgeArrow::Last:
			drawTargetArrow = true;
			break;
		case EdgeArrow::Both:
			drawTargetArrow = true;
			OGDF_CASE_FALLTHROUGH;
		case EdgeArrow::First:
			drawSourceArrow = true;
			break;
		default:
			// don't draw any arrows
			break;
		}
	}

	xmlNode = xmlNode.append_child("g");
	bool drawLabel = m_attr.has(GraphAttributes::edgeLabel) && !m_attr.label(e).empty();
	pugi::xml_node label;

	if(false) {//drawLabel
		label = xmlNode.append_child("text");
		label.append_attribute("text-anchor") = "middle";
		label.append_attribute("dominant-baseline") = "middle";
		label.append_attribute("font-family") = m_settings.fontFamily().c_str();
		label.append_attribute("font-size") = m_settings.fontSize();
		label.append_attribute("fill") = m_settings.fontColor().c_str();
		//label.text() = m_attr.label(e).c_str();
		label.text() = to_string(edge_count).c_str();
	}


	//creates a path whose only points are the two nodes that start and end the edge
	//along with anything in bends
	DPolyline path = m_attr.bends(e);
	node s = e->source();
	node t = e->target();
	path.pushFront(DPoint(m_attr.x(s), m_attr.y(s)));
	path.pushBack(DPoint(m_attr.x(t), m_attr.y(t)));

	bool drawSegment = false;
	bool finished = false;

	List<DPoint> points;

	for(ListConstIterator<DPoint> it = path.begin(); it.succ().valid() && !finished; it++) {
		DPoint p1 = *it;
		DPoint p2 = *(it.succ());

		// leaving segment at source node ?
		if(isCoveredBy(p1, e, s) && !isCoveredBy(p2, e, s)) {
			if(!drawSegment && drawSourceArrow) {
				drawArrowHead(xmlNode, p2, p1, s, e);
			}

			drawSegment = true;
		}

		// entering segment at target node ?
		if(!isCoveredBy(p1, e, t) && isCoveredBy(p2, e, t)) {
			finished = true;

			if(drawTargetArrow) {
				drawArrowHead(xmlNode, p1, p2, t, e);
			}
		}

		if(drawSegment && drawLabel) {
			label.append_attribute("x") = (p1.m_x + p2.m_x) / 2;
			label.append_attribute("y") = (p1.m_y + p2.m_y) / 2;

			drawLabel = false;
		}

		if(drawSegment) {
			points.pushBack(p1);
		}

		if(finished) {
			points.pushBack(p2);
		}
	}

	if(points.size() < 2) {
		GraphIO::logger.lout() << "Could not draw edge since nodes are overlapping: " << e << std::endl;
	} else {
		drawCurve(xmlNode, e, points);
	}
}

void SvgPrinter::drawLine(std::stringstream &ss, const DPoint &p1, const DPoint &p2) {
	ss << " M" << p1.m_x << "," << p1.m_y << " L" << p2.m_x << "," << p2.m_y;
}

void SvgPrinter::drawBezier(std::stringstream &ss, const DPoint &p1, const DPoint &p2, const DPoint &c1, const DPoint &c2) {
	ss << " M" << p1.m_x << "," << p1.m_y << " C" << c1.m_x << "," << c1.m_y << "  " << c2.m_x << "," << c2.m_y << " " << p2.m_x << "," << p2.m_y;
}

void SvgPrinter::drawBezierPath(std::stringstream &ss, List<DPoint> &points) {
	const double c = m_settings.curviness();
	DPoint cLast = 0.5 * (points.front() + *points.get(1));

	while(points.size() >= 3) {
		const DPoint p1 = points.popFrontRet();
		const DPoint p2 = points.front();
		const DPoint p3 = *points.get(1);

		const DPoint delta = p2 - 0.5 * (p1+p3);
		const DPoint c1 = p1 + c * delta + (1-c) * (p2-p1);
		const DPoint c2 = p3 + c * delta + (1-c) * (p2-p3);

		drawBezier(ss, p1, p2, cLast, c1);

		cLast = c2;
	}

	const DPoint p1 = points.popFrontRet();
	const DPoint p2 = points.popFrontRet();
	const DPoint c1 = 0.5 * (p2 + p1);

	drawBezier(ss, p1, p2, cLast, c1);
}

void SvgPrinter::drawRoundPath(std::stringstream &ss, List<DPoint> &points) {
	const double c = m_settings.curviness();

	DPoint p1 = points.front();
	DPoint p2 = *points.get(1);

	drawLine(ss, p1, .5 * ((p1+p2) + (1-c) * (p2-p1)));

	while(points.size() >= 3) {
		p1 = points.popFrontRet();
		p2 = points.front();
		DPoint p3 = *points.get(1);

		DPoint v1 = (p1 - p2);
		DPoint v2 = (p3 - p2);

		double length = std::min(v1.norm(), v2.norm()) * c / 2;

		v1 *= length / v1.norm();
		v2 *= length / v2.norm();

		DPoint pA = p2 + v1;
		DPoint pB = p2 + v2;

		drawLine(ss, 0.5 * (p1+p2), pA);
		drawLine(ss, 0.5 * (p3+p2), pB);

		DPoint vA = p2 - p1;
		DPoint vB = p3 - p1;
		bool doSweep = vA.m_x*vB.m_y - vA.m_y*vB.m_x > 0;

		ss << " M" << pA.m_x << "," << pA.m_y << " A" << length << "," << length << " 0 0 " << (doSweep ? 1 : 0) << " " << pB.m_x << "," << pB.m_y << "";
	}

	p1 = points.popFrontRet();
	p2 = points.popFrontRet();

	drawLine(ss, p2, .5 * ((p1 + p2) + (1-c) * (p1-p2)));
}

void SvgPrinter::drawLines(std::stringstream &ss, List<DPoint> &points) {
	while(points.size() > 1) {
		DPoint p = points.popFrontRet();
		drawLine(ss, p, points.front());
	}
}

pugi::xml_node SvgPrinter::drawCurve(pugi::xml_node xmlNode, edge e, List<DPoint> &points) {
	OGDF_ASSERT(points.size() >= 2);

	pugi::xml_node line = xmlNode.append_child("path");
	std::stringstream ss;

	if(points.size() == 2) {
		const DPoint p1 = points.popFrontRet();
		const DPoint p2 = points.popFrontRet();

		drawLine(ss, p1, p2);
	} else {
		if(m_settings.curviness() == 0) {
			drawLines(ss, points);
		} else if(m_settings.bezierInterpolation()) {
			drawBezierPath(ss, points);
		} else {
			drawRoundPath(ss, points);
		}
	}

	line.append_attribute("fill") = "none";
	line.append_attribute("d") = ss.str().c_str();
	appendLineStyle(line, e);

	return line;
}

DPoint* line_intersection(const DPoint &line1_p1,  //A
						  const DPoint &line1_p2,  //B
						  const DPoint &line2_p1,  //C
						  const DPoint &line2_p2)  //D
{ 
	DPoint *result = new DPoint();

	/*
		double m1 = (line1_p2.m_y - line1_p1.m_y)/(line1_p2.m_x - line1_p1.m_x);
		double m2 = (line2_p2.m_y - line2_p1.m_y)/(line2_p2.m_x - line2_p1.m_x);

		double x = ((m1*line1_p1.m_x)-(line1_p1.m_y)-(m2*line2_p1.m_x)+(line2_p1.m_y))/(m1 - m2);
		double y = (m1*x)-(m1*line1_p1.m_x)+(line1_p1.m_y);

		result->m_x = x;
		result->m_y = y;
	*/

	double x1 = line1_p2.m_y - line1_p1.m_y;
	double y1 = line1_p1.m_x - line1_p2.m_x;
	double z1 = x1*(line1_p1.m_x) + y1*(line1_p1.m_y);

	double x2 = line2_p2.m_y - line2_p1.m_y;
	double y2 = line2_p1.m_x - line2_p2.m_x;
	double z2 = x2*(line2_p1.m_x) + y2*(line2_p1.m_y);

	double d = (x1*y2) - (x2*y1);

	if(d == 0){
		std::cerr << "DETERMINATE 0\n";
		//lines are parallel
		return nullptr;
	}else{

		double x = ((y2*z1) - (y1*z2))/d;
		double y = ((x1*z2) - (x2*z1))/d;

		result->m_x = x;
		result->m_y = y;

		//std::cerr << "Line Cross Run: \n";
		//std::cerr << "\tInter: (" << x << ", " << y << ")\n";

		if(result->m_x <= std::max(line1_p1.m_x, line1_p2.m_x) &&
		   result->m_x >= std::min(line1_p1.m_x, line1_p2.m_x) &&

		   result->m_y <= std::max(line1_p1.m_y, line1_p2.m_y) &&
		   result->m_y >= std::min(line1_p1.m_y, line1_p2.m_y) &&

		   result->m_x <= std::max(line2_p1.m_x, line2_p2.m_x) &&
		   result->m_x >= std::min(line2_p1.m_x, line2_p2.m_x) &&

		   result->m_y <= std::max(line2_p1.m_y, line2_p2.m_y) &&
		   result->m_y >= std::min(line2_p1.m_y, line2_p2.m_y)
		   ){
			//std::cerr << "result: " << result->m_x << ", " << result->m_y << "\n";
			return result;
		}else{
			//std::cerr << "Failed. Didn't cross.\n";
			return nullptr; 
		}
	}
}

void SvgPrinter::drawArrowHead(pugi::xml_node xmlNode, const DPoint &start, DPoint &end, node v, edge e){
	const double dx = end.m_x - start.m_x;
	const double dy = end.m_y - start.m_y;
	const double size = getArrowSize(e, v);

	//bool source = false, target = false;

	/*
	std::cerr << "TYPE:\n";
	if(NodeElement::compare(*v, *(e->source()))){
		source = true;
		std::cerr << "\tSOURCE\n";
	}else if(NodeElement::compare(*v, *(e->target()))){
		target = true;
		std::cerr << "\tTARGET\n";
	} 
	*/

	std::cerr << "HERE\n";

	pugi::xml_node arrowHead;

	if(dx == 0) {
		int sign = dy > 0 ? 1 : -1;
		double y = m_attr.y(v) - m_attr.height(v)/2 * sign;
		end.m_y = y - sign * size;

		std::map<std::pair<node, edge>, std::string>::iterator a_type_ptr;
		a_type_ptr = m_node_arrow.find(std::make_pair(v, e));
		std::list<double> coord;
		bool hollow = false;
		std::cerr << "Arrow Type: ";
		if(a_type_ptr != m_node_arrow.end()){
			if(a_type_ptr->second == "none"){
				std::cerr << "NONE\n";
				end.m_y = y;
			}else if(a_type_ptr->second == "filled_arrow"){
				std::cerr << "FILLED ARROW\n";
				coord.push_back(end.m_x);
				coord.push_back(y);
				coord.push_back(end.m_x - size/2.5);
				coord.push_back(y - size*sign);
				coord.push_back(end.m_x + size/2.5);
				coord.push_back(y - size*sign);
			}else if(a_type_ptr->second == "hollow_arrow"){
				std::cerr << "HOLLOW ARROW\n";
				coord.push_back(end.m_x);
				coord.push_back(y);
				coord.push_back(end.m_x - size/2.5);
				coord.push_back(y - size*sign);
				coord.push_back(end.m_x + size/2.5);
				coord.push_back(y - size*sign);

				hollow = true;
			}else if(a_type_ptr->second == "filled_diamond"){
				std::cerr << "FILLED DIAMOND\n";
				coord.push_back(end.m_x);
				coord.push_back(y);

				coord.push_back(end.m_x - size/2.5);
				coord.push_back(y - size*sign);

				coord.push_back(end.m_x);
				coord.push_back(y - size*2*sign);

				coord.push_back(end.m_x + size/2.5);
				coord.push_back(y - size*sign);

				end.m_y = y - size*2*sign;
			}else if(a_type_ptr->second == "hollow_diamond"){
				std::cerr << "HOLLOW DIAMOND\n";
				coord.push_back(end.m_x);
				coord.push_back(y);

				coord.push_back(end.m_x - size/2.5);
				coord.push_back(y - size*sign);

				coord.push_back(end.m_x);
				coord.push_back(y - size*2*sign);

				coord.push_back(end.m_x + size/2.5);
				coord.push_back(y - size*sign);

				end.m_y = y - size*2*sign;

				hollow = true;
			}else{
				std::cerr << "ERROR\n";
				//Error
			}
		}

		arrowHead = drawPolygon(xmlNode, coord);
		if(hollow){
			arrowHead.append_attribute("stroke") = "#000000";
			arrowHead.append_attribute("fill-opacity") = "0";
		}

	} else {
		// identify the position of the tip
		DPoint *tip = nullptr;
		std::vector<DPoint> corners;
		corners.push_back(DPoint(m_attr.x(v), m_attr.y(v)));
		corners.push_back(DPoint(m_attr.x(v) + m_attr.width(v), m_attr.y(v)));
		corners.push_back(DPoint(m_attr.x(v) + m_attr.width(v), m_attr.y(v) + m_attr.height(v)));
		corners.push_back(DPoint(m_attr.x(v), m_attr.y(v) + m_attr.height(v)));

		for(int i = 0, j = 1; i < 4; ++i){
			tip = line_intersection(start, end, corners[i], corners[j]);
			++j; if(j == 4) j = 0;
			if(tip != nullptr){
				break;
			}
		}

		double slope = dy / dx;
		int sign = dx > 0 ? 1 : -1;

		double x = m_attr.x(v) - m_attr.width(v)/2 * sign;
		double delta = x - start.m_x;
		double y = start.m_y + delta*slope;

		if(!isCoveredBy(DPoint(x,y), e, v)) {
			sign = dy > 0 ? 1 : -1;
			y = m_attr.y(v) - m_attr.height(v)/2 * sign;
			delta = y - start.m_y;
			x = start.m_x + delta/slope;
		}

		std::cerr << "Tip Calc: ";
		if(tip != nullptr){
			std::cerr << " NEW\n";
			end.m_x = tip->m_x;
			end.m_y = tip->m_y;
		}else{
			std::cerr << " ORIGINAL\n";
			end.m_x = x;
			end.m_y = y;
		}

		//New doesn't work currently. 
		end.m_x = x;
		end.m_y = y;

		// draw the actual arrow head

		double dx2 = end.m_x - start.m_x;
		double dy2 = end.m_y - start.m_y;
		double length = std::sqrt(dx2*dx2 + dy2*dy2);
		dx2 /= length;
		dy2 /= length;

		double mx = end.m_x - size * dx2;
		double my = end.m_y - size * dy2;
	
		double x2 = mx - size/2.5 * dy2;
		double y2 = my + size/2.5 * dx2;

		double x3 = mx + size/2.5 * dy2;
		double y3 = my - size/2.5 * dx2;

		//determine arrowhead type

		//determine arrow type from m_node_arrow
		std::map<std::pair<node, edge>, std::string>::iterator a_type_ptr;
		a_type_ptr = m_node_arrow.find(std::make_pair(v, e));
		std::list<double> coord;
		bool hollow = false;
		std::cerr << "Arrow Type: ";
		if(a_type_ptr != m_node_arrow.end()){
			if(a_type_ptr->second == "none"){
				std::cerr << "NONE\n";
			}else if(a_type_ptr->second == "filled_arrow"){
				std::cerr << "FILLED ARROW\n";
				coord.push_back(end.m_x);
				coord.push_back(end.m_y);
				coord.push_back(x2);
				coord.push_back(y2);
				coord.push_back(x3);
				coord.push_back(y3);

				double vx = start.m_x - end.m_x;
				double vy = start.m_y - end.m_y;
				double v_mag = std::sqrt(vx*vx + vy*vy);
				double temp = 20;

				end.m_x += temp*(vx/v_mag);
				end.m_y += temp*(vy/v_mag);

			}else if(a_type_ptr->second == "hollow_arrow"){
				std::cerr << "HOLLOW ARROW\n";
				coord.push_back(end.m_x);
				coord.push_back(end.m_y);
				coord.push_back(x2);
				coord.push_back(y2);
				coord.push_back(x3);
				coord.push_back(y3);

				double vx = start.m_x - end.m_x;
				double vy = start.m_y - end.m_y;
				double v_mag = std::sqrt(vx*vx + vy*vy);
				double temp = 20;

				end.m_x += temp*(vx/v_mag);
				end.m_y += temp*(vy/v_mag);

				hollow = true;
			}else if(a_type_ptr->second == "filled_diamond"){
				std::cerr << "FILLED DIAMOND\n";
				coord.push_back(end.m_x);
				coord.push_back(end.m_y);
				coord.push_back(x2);
				coord.push_back(y2);

				double vx = start.m_x - end.m_x;
				double vy = start.m_y - end.m_y;
				double v_mag = std::sqrt(vx*vx + vy*vy);
				double temp = 44;

				coord.push_back(end.m_x + temp*(vx/v_mag));
				coord.push_back(end.m_y + temp*(vy/v_mag));
				end.m_x += temp*(vx/v_mag);
				end.m_y += temp*(vy/v_mag);

				coord.push_back(x3);
				coord.push_back(y3);
			}else if(a_type_ptr->second == "hollow_diamond"){
				std::cerr << "HOLLOW DIAMOND\n";
				coord.push_back(end.m_x);
				coord.push_back(end.m_y);
				coord.push_back(x2);
				coord.push_back(y2);

				double vx = start.m_x - end.m_x;
				double vy = start.m_y - end.m_y;
				double v_mag = std::sqrt(vx*vx + vy*vy);
				double temp = 40;

				coord.push_back(end.m_x + temp*(vx/v_mag));
				coord.push_back(end.m_y + temp*(vy/v_mag));
				end.m_x += temp*(vx/v_mag);
				end.m_y += temp*(vy/v_mag);

				coord.push_back(x3);
				coord.push_back(y3);

				hollow = true;
			}else{
				std::cerr << "ERROR\n";
				//Error
			}
		}

		arrowHead = drawPolygon(xmlNode, coord);
		if(hollow){
			arrowHead.append_attribute("stroke") = "#000000";
			arrowHead.append_attribute("fill-opacity") = "0";
		}
	}
	//appendLineStyle(arrowHead, e);
}
