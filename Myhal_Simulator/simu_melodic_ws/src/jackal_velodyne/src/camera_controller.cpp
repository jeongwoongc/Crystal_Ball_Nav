#include "camera_controller.hh"

namespace gazebo {

CameraController::CameraController(): CameraPlugin() {}

void CameraController::Load(sensors::SensorPtr _parent, sdf::ElementPtr _sdf) {

    CameraPlugin::Load(_parent, _sdf);

    //std::cout << "Loading Custom Camera: " << this->parentSensor->Name() << std::endl;
    print_color("Loading Custom Camera: " + this->parentSensor->Name(), EMPHBLUE);

    /*if (_sdf->HasElement("filepath")){
        this->filepath = _sdf->GetElement("filepath")->Get<std::string>();
    } else {
        this->filepath = "/tmp/";
    }*/

    int argc = 0;

    char **argv = NULL;
    
    ros::init(argc, argv, this->parentSensor->Name());
    
    this->last_update = ros::Time::now();

    if (gazebo::physics::has_world("default")){
        this->world = gazebo::physics::get_world("default");
        print_color(this->parentSensor->Name() + " found world!", EMPHBLUE);
        this->p_eng = this->world->Physics();
    } else {
        print_color(this->parentSensor->Name() + " failed to find world, aborting", EMPHRED);    
        return;
    }

    if (!this->nh.getParam("min_step", this->min_step)) {
        this->min_step = 0.0001;
    }

    std::string home_path = "/home/admin";
    if (const char *home_path0 = std::getenv("HOME"))
        home_path = home_path0;

    std::string start_time;
    if (!this->nh.getParam("start_time", start_time)){
        this->filepath = "/tmp/";
    } else{
        this->filepath = home_path + "/Deep-Collison-Checker/Data/Simulation_v2/simulated_runs/" + start_time + "/logs-" + start_time +"/videos/" + this->parentSensor->Name() + "/";
    }

    print_color("Min step size: " + std::to_string(this->min_step), EMPHGREEN);

    std::string command = "mkdir " + this->filepath;
    std::system(command.c_str());
    this->fps = (double) this->parentSensor->UpdateRate();

}

void CameraController::OnNewFrame(const unsigned char *_image, 
        unsigned int _width, 
        unsigned int _height, 
        unsigned int _depth, 
        const std::string &_format) {

    // Init
    // ****

    //std::cout << "ROS: " << this->last_update_time << " "; 
    //std::cout << "Gazebo: " << this->parentSensor->LastUpdateTime() << std::endl;
    
    if (!this->world->IsPaused())
    {
        this->world->SetPaused(true);
    }

    // Get ros time now
    ros::Time t_ros_now = ros::Time::now();

    // Controll Camera FPS
    if (this->save_count > 0)
    {
        double dt = (t_ros_now - this->last_update_time).toSec();
        double avg_n = std::max(this->save_count, 50);
        this->avg_dt = rolling_avg(this->avg_dt, dt, avg_n);
        double curr_step = this->p_eng->GetMaxStepSize();
        double frac = (this->avg_dt)/(1/this->fps);
        double new_step = curr_step/frac;

        // ROS_INFO_STREAM("Camera_dt (avg fps): " << dt << " (" << 1 / avg_dt << ")");

        // this->p_eng->SetMaxStepSize(std::max(new_step, this->min_step));
    }

    // Processing
    // **********

    char name[1024];
    snprintf(name, sizeof(name), "%s-%08.3f.jpg", this->parentSensor->Name().c_str(), t_ros_now.toSec());

    std::string filename = this->filepath + std::string(name);

    this->parentSensor->Camera()->SaveFrame(_image, _width, _height, _depth, _format, filename);


    // Update last time variables
    this->last_update_time = t_ros_now;
    this->save_count++;

    // Unpause simulation
    if (this->world->IsPaused())
    {
        this->world->SetPaused(false);
    }

}


GZ_REGISTER_SENSOR_PLUGIN(CameraController)

}


 
