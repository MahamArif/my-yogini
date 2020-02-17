// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

/**
* \brief The entry point for the Inference Engine Human Pose Estimation demo application
* \file human_pose_estimation_demo/main.cpp
* \example human_pose_estimation_demo/main.cpp
*/

#include <vector>

#include <inference_engine.hpp>
#include <opencv2/video/video.hpp>
#include <samples/ocv_common.hpp>

#include "human_pose_estimation_demo.hpp"
#include "human_pose_estimator.hpp"
#include "render_human_pose.hpp"
#include "scale_human_pose.hpp"

using namespace InferenceEngine;
using namespace human_pose_estimation;
using namespace std; 

bool ParseAndCheckCommandLine(int argc, char* argv[]) {
    // ---------------------------Parsing and validation of input args--------------------------------------

    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    if (FLAGS_h) {
        showUsage();
        showAvailableDevices();
        return false;
    }

    std::cout << "Parsing input parameters" << std::endl;

    if (FLAGS_i.empty()) {
        throw std::logic_error("Parameter -i is not set");
    }
	
	 if (FLAGS_c.empty()) {
        throw std::logic_error("Parameter -c is not set");
    }

    if (FLAGS_m.empty()) {
        throw std::logic_error("Parameter -m is not set");
    }

    return true;
}

int main(int argc, char* argv[]) {
    try {
        std::cout << "InferenceEngine: " << GetInferenceEngineVersion() << std::endl;
		cout << FLAGS_c << endl;

        // ------------------------------ Parsing and validation of input args ---------------------------------
        if (!ParseAndCheckCommandLine(argc, argv)) {
            return EXIT_SUCCESS;
        }

        HumanPoseEstimator estimator(FLAGS_m, FLAGS_d, FLAGS_pc);
        std::cout <<FLAGS_i<< std::endl;
		
        cv::VideoCapture cap;
        if (!(FLAGS_i == "cam" ? cap.open(0) : cap.open(FLAGS_i))) {
            throw std::logic_error("Cannot open input file or camera: " + FLAGS_i);
        }

        int delay = 33;
        double inferenceTime = 0.0;
        // read input (video) frame
        cv::Mat image;
        if (!cap.read(image)) {
            throw std::logic_error("Failed to get frame from cv::VideoCapture");
        }
        estimator.estimate(image);  // Do not measure network reshape, if it happened

        std::cout << "To close the application, press 'CTRL+C' here";
        if (!FLAGS_no_show) {
            std::cout << " or switch to the output window and press ESC key" << std::endl;
            std::cout << "To pause execution, switch to the output window and press 'p' key" << std::endl;
        }
        std::cout << std::endl;
        
        const size_t width  = static_cast<size_t>(image.cols);
        const size_t height = static_cast<size_t>(image.rows);

        cv::VideoWriter videoWriter;
        if (!FLAGS_o.empty()) {
            std::cout << "Output file exists" << std::endl;
            videoWriter.open(FLAGS_o+"/output.mp4", cv::VideoWriter::fourcc('A','V','C','1'), 25, cv::Size(width, height), true);
        }
        
        cv::Mat image_ref;
        cv::VideoCapture cap_ref;
        // cap_ref.open("./from_Chris/AI-Yogini-Project/GoodWarrior1flipped.jpg");
		cap_ref.open(FLAGS_c);
        cap_ref.read(image_ref);
        estimator.estimate(image_ref);
        //image_ref = cv::imread("~/pose_src/from_Chris/AI-Yogini-Project/GoodWarrior1.jpg", 1); //IMREAD_COLOR );
       
        
        do {
            double t1 = static_cast<double>(cv::getTickCount());
            std::vector<HumanPose> poses = estimator.estimate(image);
            std::cout << "Done1" << std::endl;
            std::vector<HumanPose> ref_poses = estimator.estimate(image_ref);
            int frontKneeAngle = get_angle_limb(ref_poses[0],{12,13}, {12,11}); // clockwise angle from lower to upper limb
            std::cout << "frontKneeAngle = " << frontKneeAngle<< std::endl;
            int backKneeAngle =  get_angle_limb(ref_poses[0],{9,10}, {9,8}); // clockwise angle
            std::cout << "backKneeAngle = " << backKneeAngle<< std::endl;
            std::vector<HumanPose> scaled_poses = scaleHumanPose(ref_poses, poses);
            std::cout << "Done" << std::endl;
            double t2 = static_cast<double>(cv::getTickCount());
            if (inferenceTime == 0) {
                inferenceTime = (t2 - t1) / cv::getTickFrequency() * 1000;
            } else {
                inferenceTime = inferenceTime * 0.95 + 0.05 * (t2 - t1) / cv::getTickFrequency() * 1000;
            }
            if (FLAGS_r) {
                for (HumanPose const& pose : poses) {
                    std::stringstream rawPose;
                    rawPose << std::fixed << std::setprecision(0);
                    for (auto const& keypoint : pose.keypoints) {
                        rawPose << keypoint.x << "," << keypoint.y << " ";
                    }
                    rawPose << pose.score;
                    std::cout << rawPose.str() << std::endl;
                }
            }

            if (FLAGS_no_show && FLAGS_o.empty()) {
                continue;
            }

            //renderHumanPose(poses, image);
            renderHumanPose(scaled_poses, image);
           
            //cv::Mat fpsPane(35, 155, CV_8UC3);
            //fpsPane.setTo(cv::Scalar(153, 119, 76));
            //cv::Mat srcRegion = image(cv::Rect(8, 8, fpsPane.cols, fpsPane.rows));
            //cv::addWeighted(srcRegion, 0.4, fpsPane, 0.6, 0, srcRegion);
            std::stringstream fpsSs;
            //fpsSs << "FPS: " << int(1000.0f / inferenceTime * 100) / 100.0f;
            cv::putText(image, fpsSs.str(), cv::Point(16, 32), cv::FONT_HERSHEY_COMPLEX, 0.8, cv::Scalar(0, 0, 255)); // ... */
            //cv::imshow("ICV Human Pose Estimation", image);
            
            if (!FLAGS_o.empty()) {
                cv::imwrite(FLAGS_o+"/keypoint.jpg",image);
                videoWriter.write(image);
                /* if single_image_mode: 
            cv::imwrite('output_image.jpg', image)
        else:
            vw.write(edges) */
            }
            

            int key = cv::waitKey(delay) & 255;
            if (key == 'p') {
                delay = (delay == 0) ? 33 : 0;
            } else if (key == 27) {
                break;
            }
        } while (cap.read(image));
        if (!FLAGS_o.empty()) {
            videoWriter.release();
        }
    }
    
    catch (const std::exception& error) {
        std::cerr << "[ ERROR ] " << error.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "[ ERROR ] Unknown/internal exception happened." << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "Execution successful" << std::endl;
    return EXIT_SUCCESS;
}
