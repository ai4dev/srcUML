#ifndef INCLUDED_SVG_THREE_OUTPUTTER_HPP
#define INCLUDED_SVG_THREE_OUTPUTTER_HPP


#include <svg_outputter.hpp>

class svg_three_outputter : public svg_outputter {

public:

	svg_three_outputter(){
		cg.init(g);

		cga.init(cg,
		GraphAttributes::nodeGraphics |
		GraphAttributes::edgeGraphics |
		GraphAttributes::nodeLabel |
		GraphAttributes::edgeLabel |
		GraphAttributes::nodeStyle |
		GraphAttributes::edgeStyle |
		GraphAttributes::nodeTemplate);
	}

	bool output(std::ostream& out, std::vector<std::shared_ptr<srcuml_class>> & classes){
		//transfer information from srcUML to ogdf
		srcuml_relationships relationships = analyze_relationships(classes);
		std::map<std::string, ogdf::node> class_node_map;

		SList<node> ctrl, bndr, enty;

		//Classes/Nodes
		for(const std::shared_ptr<srcuml_class> & aclass : classes){
			node cur_node = g.newNode();
			//Insert into map the node class pairing
			class_node_map.insert(std::pair<std::string, ogdf::node>(aclass->get_srcuml_name(), cur_node));

			int num_lines = 0;
			int longest_line = 0;
			cga.label(cur_node) = generate_label(aclass, num_lines, longest_line);

			cga.height(cur_node) = num_lines * 1.3 * 10;//num_lines * 50;

			cga.width(cur_node) = longest_line * .75 * 10;
			//longest_line * 10;

			std::string stereo = *(aclass->get_stereotypes().begin());

			Color& color = cga.fillColor(cur_node);

			if(stereo == "control"){
				
				color = Color(Color::Name::Chartreuse);
				ctrl.pushBack(cur_node);

			}else if(stereo == "boundary"){

				color = Color(Color::Name::Crimson);
				bndr.pushBack(cur_node);

			}else if(stereo == "entity"){

				color = Color(Color::Name::Aqua);
				enty.pushBack(cur_node);

			}
		}

		//Relationships/Edges
		for(const srcuml_relationship relationship : relationships.get_relationships()){
			//get the nodes from graph g, create edge and add apropriate info.
			ogdf::node lhs, rhs;

			//std::cerr << "Relationship: src:" << relationship.get_source() << '\n';
			//std::cerr << "Relationship: dst:" << relationship.get_destination() << '\n';


			std::map<std::string, ogdf::node>::iterator src_it = class_node_map.find(relationship.get_source());
			if(src_it != class_node_map.end()){
				lhs = src_it->second;
			}

			std::map<std::string, ogdf::node>::iterator dest_it = class_node_map.find(relationship.get_destination());
			if(dest_it != class_node_map.end()){
				rhs = dest_it->second;
			}

			ogdf::edge cur_edge = g.newEdge(lhs, rhs);//need to pass to ogdf::node types


			float &w = cga.strokeWidth(cur_edge);
			w = 2;

			StrokeType &st = cga.strokeType(cur_edge);

			const relationship_type r_type = relationship.get_type();
			switch(r_type){
			case DEPENDENCY:
				st = StrokeType::Dash;
				break;
			case ASSOCIATION:
				st = StrokeType::Solid;
				break;
			case BIDIRECTIONAL:
				st = StrokeType::Solid;
				break;
			case AGGREGATION:
				st = StrokeType::Solid;
				break;
			case COMPOSITION:
				st = StrokeType::Solid;
				break;
			case GENERALIZATION:
				st = StrokeType::Dash;
				break;
			case REALIZATION:
				st = StrokeType::Dash;
				break;
			}
		}


		cluster control = cg.createCluster(ctrl);
		cluster boundary = cg.createCluster(bndr);
		cluster entity = cg.createCluster(enty);

		cga.label(control) = "Control";
		cga.label(boundary) = "Boundary";
		cga.label(entity) = "Entity";


	/*	
		SugiyamaLayout sl;
		sl.setRanking(new OptimalRanking);
		sl.setCrossMin(new MedianHeuristic);
 
		OptimalHierarchyLayout *ohl = new OptimalHierarchyLayout;
		ohl->layerDistance(120.0);
		ohl->nodeDistance(200.0);
		ohl->weightBalancing(1);
		sl.setLayout(ohl);

		sl.call(ga);
	*/

		ClusterPlanarizationLayout cpl;
		cpl.call(g, cga, cg);

		GraphIO::SVGSettings * svg_settings = new ogdf::GraphIO::SVGSettings();
		
		if(!drawSVG(cga, out, *svg_settings)){
			std::cout << "Error Write" << std::endl;
		}


		return true;
	}

private:

	Graph g;

	ClusterGraph cg;

	ClusterGraphAttributes cga;

};

#endif