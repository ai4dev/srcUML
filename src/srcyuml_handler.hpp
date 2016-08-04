/**
 * @file srcyuml_handler.hpp
 *
 * @copyright Copyright (C) 2015-2016 srcML, LLC. (www.srcML.org)
 *
 * This file is part of srcYUML.
 *
 * srcYUML is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * srcYUML is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with srcYUML.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_SRCYUML_HANDLER_HPP
#define INCLUDED_SRCYUML_HANDLER_HPP

#include <srcSAXEventDispatchUtilities.hpp>
#include <srcSAXController.hpp>

#include <srcSAXSingleEventDispatcher.hpp>
#include <DeclTypePolicySingleEvent.hpp>

#include <iostream>
#include <iomanip>
#include <algorithm>

/**
 * srcyuml_handler
 *
 * Base class that provides hooks for processing.
 */
class srcyuml_handler : public srcSAXEventDispatch::PolicyListener {

private:

    std::ostream & out;
    std::vector<DeclTypePolicy::DeclTypeData *> classes;
    

public:

    srcyuml_handler(const std::string & input_str, std::ostream & out) : out(out) {

        srcSAXController controller(input_str);
        run(controller);

    }

    srcyuml_handler(const char * input_filename, std::ostream & out) : out(out) {

        srcSAXController controller(input_filename);
        run(controller);
        // output_yuml();

    }

    ~srcyuml_handler() {

        std::for_each(classes.begin(), classes.end(), [](DeclTypePolicy::DeclTypeData * ptr) { delete ptr; });

    }

    void run(srcSAXController & controller) {

        DeclTypePolicy class_policy{this};
        srcSAXEventDispatch::srcSAXSingleEventDispatcher<DeclTypePolicy> handler { &class_policy };
        controller.parse(&handler);

    }

    void output_yuml() {

        for(DeclTypePolicy::DeclTypeData * data : classes) 
            output_yuml_class(*data);

    }

    void output_yuml_class(const DeclTypePolicy::DeclTypeData & data) {

        // out << '[';
        out << (*data.type) << ' ' << (*data.name) << '\n';
        // for(DeclTypePolicy::ParentData p_data : data.parents) {
        //     out << "\t";
        //     out << p_data.name << ": " << p_data.isVirtual << ',' << p_data.accessSpecifier << '\n';

        // }

        // out << "]\n";
        out << '\n';

    }

    virtual void Notify(const srcSAXEventDispatch::PolicyDispatcher * policy, const srcSAXEventDispatch::srcSAXEventContext & ctx) override {

        if(typeid(DeclTypePolicy) == typeid(*policy)) {

            classes.emplace_back(policy->Data<DeclTypePolicy::DeclTypeData>());

        }

    }

private:

};

#endif
