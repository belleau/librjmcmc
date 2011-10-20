//[building_footprint_rectangle_gilviewer_parameters
#include "param/wx_parameter.hpp"
typedef parameters< wx_parameter > param;
#include "core/building_footprint_rectangle_parameters_inc.hpp"
//]

//[building_footprint_rectangle_gilviewer_core_include
#include "core/building_footprint_rectangle.hpp"
//]


//[building_footprint_rectangle_gilviewer_gilviewer_and_resources
#include <GilViewer/gui/panel_viewer.hpp>
#include <GilViewer/gui/layer_control.hpp>
#include <GilViewer/gui/panel_manager.hpp>
#include <GilViewer/plugins/plugin_base.hpp>
#include <GilViewer/layers/image_layer.hpp>
#include <GilViewer/layers/vector_layer_ghost.hpp>
#include <GilViewer/io/gilviewer_file_io.hpp>
#include <GilViewer/io/gilviewer_io_factory.hpp>
#include "gui/resources/IGN.xpm"
//]

//[building_footprint_rectangle_gilviewer_visitors
#include "simulated_annealing/visitor/composite_visitor.hpp"
#include "simulated_annealing/visitor/wx/all.hpp"
#include "simulated_annealing/visitor/any_visitor.hpp"
#include "rjmcmc/sampler/any_sampler.hpp"
//]

#include "simulated_annealing/salamon_initial_schedule.hpp"

//[building_footprint_rectangle_gilviewer_specific
#include <boost/thread.hpp>
#include <boost/filesystem/operations.hpp>
#include <wx/app.h>
#include <wx/gdicmn.h>
#include <vector>
#include <wx/aui/framemanager.h>
//]

#include <boost/filesystem.hpp>

//[building_footprint_rectangle_gilviewer_app_declaration
#include "simulated_annealing/visitor/controler.hpp"
class building_footprint_rectangle_gilviewer : public wx_plugin_base, public simulated_annealing::Controler
{
    //]

private:
    typedef rjmcmc::any_sampler<configuration> any_sampler;
    typedef simulated_annealing::any_composite_visitor<configuration,any_sampler> any_composite_visitor;

    //[building_footprint_rectangle_gilviewer_app_oninit
    bool OnInit()
    {
        if(m_confg_visitor) return true;
        /*< Parameters processing >*/
        param *p = param::instance();
        initialize_parameters(p);

        /*< Visitors initialization >*/
        panel_viewer *panel = panel_manager::instance()->panels_list()[0];
        m_confg_visitor = new simulated_annealing::wx::configuration_visitor(panel);
        m_param_visitor = new simulated_annealing::wx::parameters_visitor(panel,m_parent);
        m_chart_visitor = new simulated_annealing::wx::chart_visitor(m_parent);
        m_contr_visitor = new simulated_annealing::wx::controler_visitor(this,m_parent);
        // m_visitor.push_back( m_log_visitor);
        m_visitor.push_back( *m_confg_visitor);
        m_visitor.push_back( *m_param_visitor);
        m_visitor.push_back( *m_chart_visitor);
        m_visitor.push_back( *m_contr_visitor);

        m_contr_visitor->add_pane(m_manager);
        m_manager->Update();
        return true;
    }
    //]

public:


    std::string submenu_name() { return "librjmcmc_unused ???"; }
    std::string menuentry_name() { return "librjmcmc"; }

    virtual void show(wxCommandEvent&) {
        OnInit();
        param_visibility(true);
        chart_visibility(true);
    }

    //[building_footprint_rectangle_gilviewer_go
    virtual void process()
    {
        OnInit();
        release();
        param *p = param::instance();
        update_values(p);

        Iso_rectangle_2 bbox = get_bbox(p);
        std::string  dsm_file = p->get<boost::filesystem::path>("dsm" ).string();
        clip_bbox(bbox,dsm_file );

        gradient_functor gf(p->get<double>("sigmaD"));
        oriented_gradient_view grad_view(dsm_file,  bbox, gf);
        m_grad = grad_view.img();

        // Checks if the file is already loaded. If not load it.
        bool already_loaded = false;
        layer_control *lc = m_confg_visitor->panelviewer()->layercontrol();
        layer_control::const_iterator it  = lc->begin();
        layer_control::const_iterator end = lc->end();
        for(;it!=end;++it)
        {
            using namespace boost::filesystem;
            if((*it)->filename()==path(system_complete(dsm_file)).string())
            {
                already_loaded = true;
                break;
            }
        }
        if(!already_loaded)
        {
            panel_viewer *panel = m_confg_visitor->panelviewer();
            boost::shared_ptr<vector_layer_ghost> ghost = panel->vectorlayerghost();
            std::string extension(boost::filesystem::extension(dsm_file));
            try
            {
                boost::shared_ptr<gilviewer_file_io> file_io = m_factory->create_object(extension.substr(1,extension.size()-1));
                layer::ptrLayerType  ilayer = file_io->load(dsm_file);
                if(!ilayer) throw std::exception();
                lc->add_layer( ilayer );
                if(ghost)
                {
                    ghost->reset();
                    ghost->transform() = ilayer->transform();
                }
            }
            catch (const std::exception &e)
            {
                using namespace std;
                GILVIEWER_LOG_EXCEPTION("Read error: " + dsm_file);
                return;
            }
        }

        set_bbox(p,bbox);
        wxPoint p0(wxCoord(bbox.min().x()),wxCoord(bbox.min().y()));
        wxPoint p1(wxCoord(bbox.max().x()),wxCoord(bbox.max().y()));
        m_confg_visitor->set_bbox(wxRect(p0,p1));

        init_visitor        (p,m_visitor);
        create_configuration(p,grad_view,m_config);
        //<-
        //    estimate_initial_temperature(p,100,*m_config);
        //->
        create_sampler      (p,m_sampler);
        create_schedule     (p,m_schedule);
        create_end_test     (p,m_end_test);

    //    std::cout << "Salamon initial schedule : " << salamon_initial_schedule(m_sampler->density(),*m_config,1000) << std::endl;
    //    m_config->clear();

        any_sampler *sampler = new any_sampler(*m_sampler);

        m_thread = new boost::thread(
                simulated_annealing::optimize<configuration,any_sampler,schedule,end_test,any_composite_visitor>,
                boost::ref(*m_config), boost::ref(*sampler),
                boost::ref(*m_schedule),   boost::ref(*m_end_test),
                boost::ref(m_visitor) );
    }
    //]

    //[building_footprint_rectangle_gilviewer_controller_overload
    virtual void go  () { process(); }
    virtual void stop() { m_end_test->stop(); }

    virtual void param_visibility(bool b) { m_param_visitor->Show(b); }
    virtual void chart_visibility(bool b) { m_chart_visitor->Show(b); }
    virtual bool param_visibility() const { return m_param_visitor->IsShown(); }
    virtual bool chart_visibility() const { return m_chart_visitor->IsShown(); }
    //]

    //[building_footprint_rectangle_gilviewer_cons_des
    building_footprint_rectangle_gilviewer() :
            m_config(NULL), m_sampler(NULL), m_schedule(NULL), m_end_test(NULL),
            m_visitor(), m_thread(NULL),
            m_confg_visitor(NULL), m_param_visitor(NULL), m_chart_visitor(NULL)
    {
    }

    virtual ~building_footprint_rectangle_gilviewer() {
        release();
    }
    //]

virtual bool Register(gilviewer_io_factory *factory)
    {
        m_factory = factory;
    }

private:
    //[building_footprint_rectangle_gilviewer_release
    void release()
    {
        if(m_end_test){ m_end_test->stop(); }
        if(m_thread)  { m_thread->join(); delete m_thread; m_thread=NULL; }
        if(m_end_test){ delete m_end_test;m_end_test=NULL; }
        if(m_config)  { delete m_config;  m_config  =NULL; }
        if(m_schedule){ delete m_schedule;m_schedule=NULL; }
        if(m_sampler) { delete m_sampler; m_sampler =NULL; }
    }
    //]

    //[building_footprint_rectangle_gilviewer_members
    configuration *m_config;
    sampler       *m_sampler;
    schedule      *m_schedule;
    end_test      *m_end_test;
    any_composite_visitor  m_visitor;
    boost::thread *m_thread;

    gilviewer_io_factory *m_factory;

    simulated_annealing::wx::controler_visitor     *m_contr_visitor;
    simulated_annealing::wx::configuration_visitor *m_confg_visitor;
    simulated_annealing::wx::parameters_visitor    *m_param_visitor;
    simulated_annealing::wx::chart_visitor         *m_chart_visitor;
    simulated_annealing::wx::log_visitor            m_log_visitor;
    boost::shared_ptr<gradient_image_t>     m_grad;
};
//]

//[building_footprint_rectangle_gilviewer_implement
IMPLEMENT_PLUGIN(building_footprint_rectangle_gilviewer);
//]
