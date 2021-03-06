//
//  find_components.cpp
//  PaperPixelCode
//
//  Created by Adarsh Kosuru on 6/30/14.
//  Copyright (c) 2014 Frog. All rights reserved.
//

#ifndef NDEBUG
#define Check(x) assert(x)
#define CheckWithMessage(str,x) assert((str,x))
#endif

#include "find_components.h"

namespace ppc {
    Components::Components():path(""){}
    Components::Components(std::string& dir_path):
    
        path(dir_path),
        no_of_threshold_levels(1),
        lower_thresh(0),
        upper_thresh(255),
        counter(0),
        extracted_images_counter(0),
        area(1000),
        width(500),
        height(500),
        edge_image(lower_thresh,upper_thresh),
        window_size(CV_WINDOW_AUTOSIZE),
        display_extracted_images(false),
        contours_image(CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE)
    {}
    
    Components::~Components(){}
    
    double Components::angle( Point pt1, Point pt2, Point pt0 )
    {
        double dx1 = pt1.x - pt0.x;
        double dy1 = pt1.y - pt0.y;
        double dx2 = pt2.x - pt0.x;
        double dy2 = pt2.y - pt0.y;
        return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
    }
    
    void Components::print_values(){
        cout << "lower thresh - " << lower_thresh << "upper thresh - " << upper_thresh << endl;
        cout << "path - " << path << endl;
    }
    
    void Components::set_output_path(string dir){
        this->output_path = dir;
    }
    
    void Components::test_method(){
        Rect r(107,1006,236,36);
        Mat new_img(source_image,r);
        imwrite(output_path + "/testfunc.jpg", new_img);
    }
    
    void Components::init(){
        
        this->find();
//        this->save_images();
        //this->ocr();
        //        Components::do_image_magick();
        //        Components::ocr_image_magick();
//        this->test_method();

        
        /*
        Components::find();
        Components::ocr();
//        Components::do_image_magick();
//        Components::ocr_image_magick();
        Components::test_method();
         */
    }
    
    void Components::mini_watershed_for_thresholding(){
        
        CheckWithMessage(std::string("Set path during initialization before finding components"), path != "");
        LoadImage load_source_image(this->path,"SquaresImage");
        source_image = load_source_image.get_image();
        CheckWithMessage(std::string("Image loaded incorrectly"), source_image.data);
        
        source_image.copyTo(source_image_resized);
        load_source_image.set_image(source_image_resized);
        source_image_resized = load_source_image.get_image();
        
        resize(source_image_resized,source_image_resized,Size(0,0),0.5,0.5);
        
        Mat binary;
        cvtColor(source_image_resized, binary, CV_BGR2GRAY);
        threshold(binary, binary, 100, 255, CV_THRESH_BINARY);
        
        
        Mat fg,bg;
        erode(binary, fg, Mat(),Point(-1,-1),3);
        
        dilate(binary, bg, Mat(),Point(-1,-1),3);
        threshold(bg, bg, 1, 128, CV_THRESH_BINARY_INV);
        
        
        Mat markers(binary.size(),CV_8U,Scalar(0));
        markers = fg + bg;
        
        WatershedMarker wm;
        wm.set_markers(markers);
        
        Mat res,temp_res;
        res = wm.apply_watershed(source_image_resized);
        res.convertTo(res, CV_8U);
        
        
        Mat res_canny,res_copy,res_output;
        res.copyTo(res_copy);
        Canny(res_copy, res_canny, 100, 255);
        
        Mat image_contours;
        std::vector<std::vector<cv::Point>> main_contours;
        vector<cv::Vec4i> hierar;
        vector<int> indexes;
        Scalar color(0,255,0);
        findContours(res_canny, main_contours, squares_hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);
        res_output = Mat::zeros(res_canny.size(), CV_8UC3);
        
        
//        cout << "Num of watershed contours - " << main_contours.size() << endl;
        
        drawContours(res_output, main_contours, -1, color,2,8,hierar,0,Point());
        
        imshow("resOutput", res_output);
        
        for (int i=0; i<squares_hierarchy.size(); i++) {
            //        std::cout  << hierarchy[i] << std::endl;
            if (squares_hierarchy[i][3] == -1) {
                hierar.push_back(squares_hierarchy[i]);
                indexes.push_back(i);
            }
        }
        
        for (vector<int>::iterator itt = indexes.begin();itt!=indexes.end();++itt) {
            needed_contours.push_back(main_contours[*itt]);
        }
        
        int highest_area = 0,second_highest_area=0;
        Rect main_rect,second_highest_area_rect;
        vector<int> areas;
        vector<int> inserted_areas;
        vector<Rect> sorted_rectangles;
        for (vector<int>::iterator itt = indexes.begin();itt!=indexes.end();++itt) {
            Rect r = boundingRect(Mat(main_contours[*itt]));
            //            cout << "area[" << *itt << "] - "<<r.area() <<endl;
            areas.push_back(r.area());
            all_rectangles.push_back(r);
            
            
            if (r.area() > highest_area) {
                second_highest_area=highest_area;
                highest_area = r.area();
                second_highest_area_rect = main_rect;
                main_rect = r;
            }
            
        }
        
        
        for (vector<Rect>::iterator rect_iterator = all_rectangles.begin(); rect_iterator!=all_rectangles.end(); ++rect_iterator) {
            
            if (inserted_areas.size()==0) {
                inserted_areas.push_back(rect_iterator->area());
                sorted_rectangles.push_back(*rect_iterator);
            }
            
            else{
                
                
                for (int sr=0; sr<sorted_rectangles.size(); sr++) {
                    if (rect_iterator->area() > sorted_rectangles[sr].area() && sr!=sorted_rectangles.size()-1) {
                        continue;
                    }
                    
                    else{
                        sorted_rectangles.insert(sorted_rectangles.begin() + sr, *rect_iterator);
                        break;
                    }
                }
                
            }
            
        }
        
        
        
        sort(areas.begin(), areas.end());
        sort(all_rectangles.begin(),all_rectangles.end(),mysortfunction);
        
        
        for (vector<Rect>::iterator sr=sorted_rectangles.begin(); sr!=sorted_rectangles.end(); ++sr) {
            cout << "sorted_rectangles_area[" << sr-sorted_rectangles.begin() <<"] - " <<sr->area() <<endl;
        }
        
        for (vector<Rect>::iterator ar=all_rectangles.begin(); ar!=all_rectangles.end(); ++ar) {
            cout << "all_rectangles_area[" << ar-all_rectangles.begin() <<"] - " <<ar->area() <<endl;
        }
        
        for (vector<int>::iterator areas_vector = areas.begin(); areas_vector!=areas.end(); ++areas_vector) {
            cout << "areas[" << areas_vector-areas.begin() <<"]- " << *areas_vector <<endl;
        }
        
        
        cout << "all_rectangles_size - " <<all_rectangles.size()<<endl;
        Mat cop = source_image_resized(second_highest_area_rect);
        imwrite(output_path + "/cop.jpg", cop);
        imshow("Cop", cop);
        
        cop.copyTo(source_image);
        source_image.copyTo(source_image_resized);
        source_image_resized.copyTo(source_image_output);
        source_image_resized.copyTo(source_image_hull);
        
        //this->find_test();
    }
    
    void Components::chamfer_matching(string src_path,string matching_image) throw(cv::Exception){
        Mat original_text = imread(src_path);
        Mat matched_image = imread(matching_image);
        
        if (original_text.empty() || matched_image.empty() ) {
            cout << "Images not found" << endl;
        }
        
        Mat original_gray,matched_gray,cimg;
        original_text.copyTo(original_gray);
        matched_image.copyTo(matched_gray);
        
//        cvtColor(original_text, original_gray, COLOR_BGR2GRAY);
//        cvtColor(matched_image, matched_gray, COLOR_BGR2GRAY);
        cvtColor(original_gray, cimg, COLOR_GRAY2BGR);
        
        
//        Canny(original_gray, original_gray, 5, 50,3);
//        Canny(matched_gray, matched_gray, 5, 50,3);
        
        vector<vector<Point> > results;
        vector<float> costs;
        int best = chamerMatching(original_gray, matched_gray, results, costs);
        
        if (best<0) {
            cout << "Matchcing not found" << endl;
        }
        
        size_t i,n = results[best].size();
        
        for (i=0; i<n; i++) {
            Point pt = results[best][i];
            if (pt.inside(Rect(0,0,cimg.cols,cimg.rows))) {
                cimg.at<Vec3b>(pt) = Vec3b(0,255,0);
            }
        }
        
        imshow("result", cimg);
        
    }
    
    void Components::basic_thresholding_method() throw(cv::Exception){
        CheckWithMessage(std::string("Set path during initialization before finding components"), path != "");
        LoadImage load_source_image(this->path,"SquaresImage");
        source_image = load_source_image.get_image();
        CheckWithMessage(std::string("Image loaded incorrectly"), source_image.data);
        
        Mat src_gray,resized_img;
        source_image.copyTo(resized_img);
        resize(resized_img, resized_img, Size(0,0),0.5,0.5);
        
        cvtColor(resized_img, src_gray, COLOR_BGR2GRAY);
        blur(src_gray, src_gray, Size(3,3));
        
//        imshow("Blurred", src_gray);
        
        Mat threshold_output;
        vector<vector<Point> >contours;
        vector<Vec4i> hierarchy;
        
        cv::threshold(src_gray, threshold_output, 230, 255, THRESH_BINARY);
        findContours(threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE,Point(0,0));
        
        vector<RotatedRect> minRect(contours.size());
        
        for (size_t i=0; i<contours.size(); i++) {
            minRect[i] = minAreaRect(Mat(contours[i]));
        }
        
        //Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);
        Mat drawing;
        resized_img.copyTo(drawing);
        
        RNG rng(12345);
        Rect rt;
        
        for (size_t i=0; i<contours.size(); i++) {
            Scalar color = Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255));
            drawContours(drawing, contours, (int)i, color,1,8,vector<Vec4i>(),0,Point());
            
            Point2f rect_points[4];
            minRect[i].points(rect_points);
            rt = minRect[i].boundingRect();
            Mat contours_cropped(resized_img,rt);
            string out_path = this->output_path + "/" + to_string(i) + ".jpg";
            imwrite(out_path, contours_cropped);
            
//            for (int j=0; j<4; j++) {
//                line(drawing, rect_points[j], rect_points[(j+1)%4], color,1,8);
//            }
        }
        
        imwrite(this->output_path + "/drawing.jpg",drawing);
//        imshow("Drawing", drawing);
//        imshow("Cropped", contours_cropped);
    }
    
    void Components::find_test() throw(cv::Exception){
        
        cv::Mat gray0(source_image_resized.size(), CV_8U),gray;
        
        cv::medianBlur(source_image_resized, source_image_blurred, 0);
        
        std::vector<std::vector<cv::Point>> contours;
        
        squares.clear();
        
        squares_method(gray0,gray,contours);
    }
    
    void Components::find() throw(cv::Exception){
        CheckWithMessage(std::string("Set path during initialization before finding components"), path != "");
        LoadImage load_source_image(this->path,"SquaresImage");
        source_image = load_source_image.get_image();
        CheckWithMessage(std::string("Image loaded incorrectly"), source_image.data);
//        cv::resize(source_image, source_image_resized, cv::Size(0,0), 0.50,0.50 );
        source_image.copyTo(source_image_resized);
        load_source_image.set_image(source_image_resized);
        
//        load_source_image.show();
        source_image_resized.copyTo(source_image_output);
        source_image_resized.copyTo(source_image_hull);
        
        cv::Mat gray0(source_image_resized.size(), CV_8U),gray;
        
        cv::medianBlur(source_image_resized, source_image_blurred, 0);
        //    cv::GaussianBlur(source_image_resized,blurred_image,Size(1,1),0);
//        cv::namedWindow("Blurred",CV_WINDOW_AUTOSIZE);
//        cv::imshow("Blurred", source_image_blurred);
        std::vector<std::vector<cv::Point>> contours;
        
        squares.clear();
        
        //    namedWindows();
        
        //    createTrackbar( "Threshold","ContourResult", &thresh,maxThreshold, changeThresh);
        
        
        squares_method(gray0,gray,contours);
        
//        cv::waitKey(0);
    }
    
    void Components::squares_method(cv::Mat& gray0,cv::Mat& gray,std::vector<std::vector<cv::Point>>& contours) throw(cv::Exception){
        
        vector<cv::Vec4i> parent_hierarchy;
        vector<int> indexes;
        
        for (int channels=0;channels < 3; channels++) {
            int mixChannelsArray[] = {channels,0};
            
            mixChannels(&source_image_blurred, 1, &gray0, 1, mixChannelsArray, 1);
            
            for (int threshold_level =0 ; threshold_level<no_of_threshold_levels; threshold_level++) {
                if (threshold_level ==0) {
                    gray = edge_image.set_source_image(gray0).set_aperture_size(5).applyCanny().get_result_image();
                    dilate(gray, gray, Mat(),Point(-1,-1));
                }
                else{
                    gray = gray0 >= (threshold_level + 1) * 255 / no_of_threshold_levels;
                }
                
                contours.clear();
                squares_hierarchy.clear();
                parent_hierarchy.clear();
                indexes.clear();
                needed_contours.clear();
                contours = contours_image.set_source_image(gray).find().get_contours();
                squares_hierarchy = contours_image.get_hierarchy();
                
                
                //            cout << "contour size - " << contours.size() << endl;
                //            cout << "hierarchy size - " << squares_hierarchy.size() << endl;
                
                for (vector<vector<Point>>::iterator itr = contours.begin(); itr!=contours.end(); ++itr) {
                    //cout << "contours - " << *itr << endl;
                }
                
                Mat drawing_contours = Mat::zeros( gray.size(), CV_8UC3 );
                RNG rng(12345);
                for (size_t i=0; i<contours.size(); i++) {
                    Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
                    // contour
                    drawContours( drawing_contours, contours, (int)i, color, 1, 8, vector<Vec4i>(), 0, Point() );
                }
                
                imshow("DrawingContours",drawing_contours);
                
                for (vector<cv::Vec4i>::iterator itr = squares_hierarchy.begin(); itr!=squares_hierarchy.end(); ++itr) {
                                    //cout << "square_hierarchy - "<< int(itr-squares_hierarchy.begin())<< "  th level - "<< threshold_level << " - "<<*itr << endl;
                    //counter++;
                }
                
                //            cout << "counter - " << counter << endl;
                //            counter = 0;
                
                
                for (int i=0; i<squares_hierarchy.size(); i++) {
                    //        std::cout  << hierarchy[i] << std::endl;
                    if (squares_hierarchy[i][3] == -1) {
                        parent_hierarchy.push_back(squares_hierarchy[i]);
                        indexes.push_back(i);
                        //                    needed_contours.push_back(contours[indexes.size()-1]);
                        //            cout << "parent here - " << endl;
                        //            std::cout  << hierarchy[i] << std::endl;
                    }
                }
                
                
                for (vector<int>::iterator itt = indexes.begin();itt!=indexes.end();++itt) {
                    needed_contours.push_back(contours[*itt]);
                }
                
                
                Mat drawing_needed_contours = Mat::zeros( gray.size(), CV_8UC3 );
                RNG rng_needed(12345);
                for (size_t i=0; i<needed_contours.size(); i++) {
                    Scalar color = Scalar( rng_needed.uniform(0, 255), rng_needed.uniform(0,255), rng_needed.uniform(0,255) );
                    // contour
                    drawContours( drawing_needed_contours, needed_contours, (int)i, color, 1, 8, vector<Vec4i>(), 0, Point() );
                }
                
                
                //imwrite(this->output_path + "/needed" +to_string(threshold_level) + ".jpg", drawing_needed_contours);
                imshow("DrawingNeededContours",drawing_needed_contours);
                
                
                //            cout << "indexes size - " << indexes.size() << endl;
                //            cout << "parent hierarchy size - " << parent_hierarchy.size() << endl;
                
                for (int i=0; i<parent_hierarchy.size(); i++) {
                    //         std::cout  << parent_hierarchy[i] << "index - " << indexes[i] <<std::endl;
                }
                
                for (vector<int>::iterator itt = indexes.begin();itt!=indexes.end();++itt) {
                    
                    //                cout << "selected contours - " << *itt<<  " - "<<contours[*itt] << endl;
                    //                needed_contours.push_back(contours[*itt]);
                    cv::Rect r = cv::boundingRect(cv::Mat(contours[*itt]));
                    //        cout << "rect width & height - " << itr - indexes.begin() << r.width << " , " << r.height << endl;
                    //        cout << "rect area - " << itr - indexes.begin() << r.area()  << endl;
                    
                    if (r.area() > area || r.width > width || r.height > height) {
                        all_rectangles.push_back(r);
                        //                    rectangle(source_image_output, r, cv::Scalar(255), 2);
                        
                        //if (flag == 0) {
//                        Mat cop(source_image_resized,r);
//                        cop.copyTo(copied);
//                        extracted_images.push_back(copied);
                        //cout << "r.x - " << r.tl().x << ",  r.y - " << r.tl().y << ",  r.width - " << r.width << ",  r.height - " << r.height << endl;
                        //source_image_resized.copyTo(copied(Rect(r.tl().x,r.tl().y,r.width,r.height)));
                        
                        //    flag =1;
                        //}
                    }
                    
                }
                
                
                // Hull Method
                
                vector<vector<Point> >hull( needed_contours.size() );
                for( int i = 0; i < needed_contours.size(); i++ )
                {  convexHull( Mat(needed_contours[i]), hull[i], false ); }
                
                /// Draw contours + hull results
                Mat drawing = Mat::zeros( gray.size(), CV_8UC3 );
                for( int i = 0; i< needed_contours.size(); i++ )
                {
                    
                    Scalar color(0,255,0);
                    //Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
                    drawContours( drawing, hull, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
                }
                
                for (int i=0; i<hull.size(); i++) {
                    //        cout << "hull - " << i << " - " << hull[i] << endl;
                }
                
                Mat rect_hull = cv::Mat::zeros(gray.size(), CV_8UC3);
                for (vector<vector<Point>>::iterator it=hull.begin(); it!=hull.end(); ++it) {
                    Rect rh = boundingRect(Mat(*it));
                    
                    /*
                    hull_rectangles.push_back(rh);
                    Mat cop(source_image_resized,rh);
                    extracted_images.push_back(cop);
                    string out_path = output_path + "/" + to_string((int)(it-hull.begin())) + ".jpg";
//                    imwrite(out_path, cop);
                     */
                    
                    
                    if (rh.area() > 100 && rh.area()<200000 && rh.width > 5 && rh.height > 5) {
                    hull_rectangles.push_back(rh);
                        //cout << "rectangle - top left - " << rh.tl().x << endl;
                        //cout << "rectangle - width - " << rh.width << " height - "<< rh.height<< endl;
                    rectangle(source_image_hull, rh, cv::Scalar(255), 2);
                    Mat cop(source_image_resized,rh);
//                    imshow("extracted"+to_string((int)(it-hull.begin())),cop);
                    string out_path = output_path + "/" + to_string((int)(it-hull.begin())) + ".jpg";
                    imwrite(out_path, cop);
                    //cop.copyTo(copied);
                    mini_ocr(cop, counter);
                    counter++;
                    extracted_images.push_back(cop);
                    //                rectangle(rect_hull, rh, cv::Scalar(255), 2);
                    
            }
                    
                }
                
                /// Show in a window
                //            namedWindow( "Hull Contours", CV_WINDOW_AUTOSIZE );
                //            imshow( "Hull Contours", drawing );
                //            namedWindow( "HullRect", CV_WINDOW_AUTOSIZE );
                //            imshow( "HullRect", rect_hull );
                
                //            namedWindow("RectOutput",CV_WINDOW_AUTOSIZE);
                //            imshow("RectOutput", source_image_output);
                
                
                
                //End of Hull
                
                
                vector<Point> approx;
                for (vector<int>::iterator itt = indexes.begin();itt!=indexes.end();++itt) {
                    //            for (size_t i=0; i < contours.size(); i++) {
                    //                approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true) * 0.02, true);
                    approxPolyDP(Mat(contours[*itt]), approx, arcLength(Mat(contours[*itt]), true) * 0.02, true);
                    
                    
                    if (approx.size() == 4 && fabs(contourArea(Mat(approx))) > 100 && isContourConvex(Mat(approx))) {
                        double maxCosine = 0;
                        
                        for (int j=2; j<5; j++) {
                            double cosine = fabs(angle(approx[j%4],approx[j-2],approx[j-1]));
                            maxCosine = MAX(maxCosine, cosine);
                            
                            if (maxCosine < 0.3) {
                                squares.push_back(approx);
                            }
                        }
                    }
                    
                }
                
                
            }
            
            //        namedWindow("Gray",CV_WINDOW_AUTOSIZE);
            //        imshow("Gray", gray);
        }
        
        for (size_t i=0; i<squares.size(); i++) {
//            const Point* p = &squares[i][0];
//            int n = (int)squares[i].size();
            //polylines(source_image_resized, &p, &n, 1, true, Scalar(0,255,0),3,CV_AA);
        }
        
//        namedWindow("ParentContours",CV_WINDOW_AUTOSIZE);
//        imshow("ParentContours", source_image_resized);
        
        
        
        //cout << "all rectangles size - " << all_rectangles.size()<<endl;
        cout << "extracted_images size - " << extracted_images.size()<<endl;
        
        for (vector<Rect>::iterator rec=all_rectangles.begin(); rec!=all_rectangles.end(); ++rec) {
            rectangle(source_image_output, *rec, cv::Scalar(255), 2);
        }
        
        for (vector<Rect>::iterator rec=hull_rectangles.begin(); rec!=hull_rectangles.end(); ++rec) {
//            rectangle(source_image_hull, *rec, cv::Scalar(255), 2);
        }
        
        save_images();
        read_ocr();
        
        if (display_extracted_images) {
            
            
            for (vector<Mat>::iterator exim=extracted_images.begin(); exim!=extracted_images.end(); ++exim) {
                string image_name = "Image"+to_string(extracted_images_counter);
                namedWindow(image_name,CV_WINDOW_AUTOSIZE);
                imshow(image_name, *exim);
                extracted_images_counter++;
            }
        }
        
//        show_image_windows();
        
        /*
        namedWindow("AllRectangles",CV_WINDOW_AUTOSIZE);
        imshow("AllRectangles", source_image_output);
        namedWindow("HullRectangles",CV_WINDOW_AUTOSIZE);
        imshow("HullRectangles", source_image_hull);
         */
        
        //    namedWindow("Copied",CV_WINDOW_AUTOSIZE);
        //    imshow("Copied", copied);
        
    }
    
    
    void Components::show_image_windows(){
        
        namedWindow("ParentContours",CV_WINDOW_AUTOSIZE);
        imshow("ParentContours", source_image_resized);
        
        namedWindow("AllRectangles",CV_WINDOW_AUTOSIZE);
        imshow("AllRectangles", source_image_output);
        namedWindow("HullRectangles",CV_WINDOW_AUTOSIZE);
        imshow("HullRectangles", source_image_hull);
    }
    
    void Components::save_image(string dir){
        imwrite( dir + "/image_result_squarebased.jpg", source_image_output);
        imwrite( dir + "/image_result_hull.jpg", source_image_hull);
    }
    
    void Components::save_images(){
        for (int i=0; i<extracted_images.size(); i++) {
            string out_path = output_path + "/" + to_string(i) + ".jpg";
            imwrite(out_path, extracted_images[i]);
        }
    }
    
    void Components::read_ocr(){
        
        cout << "\n----------------- Read ocr here -----------------\n" << endl;
        
        
        
        Mat resized_img;
        
        for (int r=0; r<extracted_images.size(); r++) {
            char *tesseract_text;
            string out_path = output_path + "/" + to_string(r) + ".jpg";
            
//            Mat input_img = imread(out_path);
            char *tess_path = (char*)out_path.c_str();
            Pix *read_image = pixRead(tess_path);
            
//            if (!input_img.data) {
//                cout << "Error no input image" << endl;
//            }
//            
//            input_img.copyTo(resized_img);
            
            //imshow("readOCR"+to_string(r),resized_img);
            
            TessBaseAPI api;
            
            if (api.Init(NULL, "eng")) {
                fprintf(stderr, "Tesseract API not initialised");
                exit(1);
            }
            
            api.SetImage(read_image);
            tesseract_text = api.GetUTF8Text();
            printf("%d. - %s\n",r,tesseract_text);
            api.End();
            delete [] tesseract_text;
            pixDestroy(&read_image);
            
//            api.SetImage((uchar*)resized_img.data, resized_img.size().width, resized_img.size().height, resized_img.channels(), resized_img.step1());
//            api.Recognize(0);
//            output_text = api.GetUTF8Text();
//            printf("%d. - %s\n",r,output_text);
//            api.End();
        }
    }
    
    void Components::mini_mini_ocr(){
        
        for (int i=0; i<164; i++) {
            Mat image = imread("/Users/adarsh.kosuru/Desktop/pixelTrial/DerivedData/pixelTrial/Build/Products/Debug/basic_thresholding/comps/" + to_string(i) + ".jpg");
            
            
            Mat grayscale_image, resized_img,thresholded_image,structure_img,sharpened_image;
            
            cvtColor(image, grayscale_image, CV_BGR2GRAY);
            threshold(grayscale_image, thresholded_image, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
            //        medianBlur(thresholded_image, thresholded_image, 0);
            GaussianBlur(thresholded_image, sharpened_image, Size(0,0), 3);
            addWeighted(thresholded_image, 1.5, sharpened_image, -0.5, 0, sharpened_image);
            resize(sharpened_image, resized_img, resized_img.size(),5,5,INTER_LINEAR);
            
            char *tesseract_text;
            TessBaseAPI api;
            
            if (api.Init(NULL, "eng")) {
                fprintf(stderr, "Tesseract API not initialised");
                exit(1);
            }
         
            api.SetImage((uchar*)resized_img.data, resized_img.size().width, resized_img.size().height, resized_img.channels(), int(resized_img.step1()));
            api.Recognize(0);
            tesseract_text = api.GetUTF8Text();
            printf("%d. - %s\n",i,tesseract_text);
            api.End();
            delete [] tesseract_text;
            
        }
    }
    
    void Components::mini_ocr(cv::Mat& ocr_image,int j){
        Mat grayscale_image, resized_img,thresholded_image,structure_img,sharpened_image;
        char *tesseract_text;
        
        cvtColor(ocr_image, grayscale_image, CV_BGR2GRAY);
//        adaptiveThreshold(grayscale_image, thresholded_image, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY,11, 2);
        threshold(grayscale_image, thresholded_image, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
//        medianBlur(thresholded_image, thresholded_image, 0);
        GaussianBlur(thresholded_image, sharpened_image, Size(0,0), 3);
        addWeighted(thresholded_image, 1.5, sharpened_image, -0.5, 0, sharpened_image);
        resize(sharpened_image, resized_img, resized_img.size(),5,5,INTER_LINEAR);
        
//        structure_img = getStructuringElement(MORPH_ELLIPSE, Size(3,3));
//        morphologyEx(thresholded_image, resized_img, MORPH_OPEN, structure_img);
//        thresholded_image.copyTo(resized_img);
        //grayscale_image.copyTo(resized_img);
//        ocr_image.copyTo(resized_img);
        
        //imshow("extracted"+to_string(j),resized_img);
        
        TessBaseAPI api;
        
        if (api.Init(NULL, "eng")) {
            fprintf(stderr, "Tesseract API not initialised");
            exit(1);
        }
        
        api.SetImage((uchar*)resized_img.data, resized_img.size().width, resized_img.size().height, resized_img.channels(), int(resized_img.step1()));
        api.Recognize(0);
        tesseract_text = api.GetUTF8Text();
        printf("%d. - %s\n",j,tesseract_text);
        api.End();
        delete [] tesseract_text;
    }
    
    void Components::ocr(){
        
//        int expanded_size=10;  //7 works for Hello //10 for text //20 for box panel
        //        int i=3;
        
        
        for (int i=0; i<20; i++) {
//        for (int i=0; i<14; i++) {
//        for (int i=0; i<3; i++) {
//          for (int i=0; i<extracted_images.size(); i++) {
        
            Mat gray_img,adapThresh,adapThresh8bit;
            Mat resized_img;
            /*
        Mat resized_img(extracted_images[i].rows*expanded_size,extracted_images[i].cols*expanded_size,extracted_images[i].type());
        cvtColor(extracted_images[i], gray_img, CV_BGR2GRAY);
        resize(gray_img, resized_img, resized_img.size(),expanded_size,expanded_size,INTER_LINEAR);
             */
        
//            gray_img.copyTo(resized_img);
            extracted_images[i].copyTo(adapThresh);
            cvtColor(adapThresh, gray_img, CV_BGR2GRAY);
//            imshow("extracted"+to_string(i),resized_img);
//            adapThresh.convertTo(adapThresh8bit, CV_8U);
//            adaptiveThreshold(gray_img, resized_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 3, 1);
            threshold(gray_img, resized_img, 0, 255, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);
            imshow("extracted"+to_string(i),resized_img);
              
        TessBaseAPI api;
            
        if (api.Init(NULL, "eng")) {
            fprintf(stderr, "Tesseract API not initialised");
            exit(1);
        }
        
//        api.SetImage((uchar*)extracted_images[0].data, extracted_images[0].size().width, extracted_images[0].size().height, extracted_images[0].channels(), extracted_images[0].step1());
        api.SetImage((uchar*)resized_img.data, resized_img.size().width, resized_img.size().height, resized_img.channels(), int(resized_img.step1()));
        api.Recognize(0);
        output_text = api.GetUTF8Text();
        printf("%d. - %s",i,output_text);
        api.End();
//        char *cstr = &output_path[0];
        //        const char *cstr = output_path.c_str();
//        printf("%s - char",cstr);
        //        Pix *image = pixRead(cstr);
            
        }
    }
    
    void Components::do_image_magick(){
        InitializeMagick("");
        string resized_size_300 = "300%";
        string resized_size_500 = "500%";
        
        for(int i=0;i<13;i++){
        
        Magick::Image image;
        
        string images_path = output_path + "/" + to_string(i) + ".jpg";
        //        const char *cstr = output_path.c_str();
        //        printf("%s - char",cstr);
        
        try {
            image.read(images_path);
            image.type(GrayscaleType);
            image.resize(resized_size_300);
            image.write(output_path + "/" + to_string(i) + "_gray" + resized_size_300 + ".jpg");
            image.resize(resized_size_500);
            image.write(output_path + "/" + to_string(i) + "_gray" + resized_size_500 + ".jpg");
            
        } catch (Magick::Exception &err) {
            cout << "Exception" << err.what() << endl;
        }
            
        }
    }
    
    void Components::ocr_image_magick(){
        
        string resized_size_300 = "300%";
        string resized_size_500 = "500%";
        
        
        cout << " ------------------- ------------------- Using ImageMagick ------------------- -------------------" << endl;
        
        for(int i=0;i<13;i++){
        
        
        TessBaseAPI api,api2;
        
        if (api.Init(NULL, "eng") || api2.Init(NULL, "eng")) {
            fprintf(stderr, "Tesseract API not initialised");
            exit(1);
        }
        
        string d_path_300 = output_path + "/" + to_string(i) + "_gray" + resized_size_300 + ".jpg";
        string d_path_500 = output_path + "/" + to_string(i) + "_gray" + resized_size_300 + ".jpg";
        const char *dir_path = &d_path_300[0];
        
        Pix *px = pixRead(dir_path);
        api.SetImage(px);
        api.Recognize(0);
        output_text = api.GetUTF8Text();
        printf("%s",output_text);
        api.End();
            
            const char *dir_path2 = &d_path_500[0];
            Pix *px2 = pixRead(dir_path2);
            api2.SetImage(px2);
            api2.Recognize(0);
            output_text = api2.GetUTF8Text();
            printf("%s",output_text);
            api2.End();
            
        }
    }
    
    void Components::find_grabcut() throw(cv::Exception){
        
    }
    
    void Components::find_watershed() throw(cv::Exception){
        
        bool show_windows = true;
        
        CheckWithMessage(std::string("Set path during initialization before finding components"), path != "");
        LoadImage load_source_image(this->path,"SquaresImage");
        source_image = load_source_image.get_image();
        CheckWithMessage(std::string("Image loaded incorrectly"), source_image.data);
        Mat rotated_image;
        cv::resize(source_image, source_image_resized, cv::Size(0,0), 0.50,0.50 );
//        rotate_image(source_image, -90, rotated_image);
//        cv::resize(rotated_image, source_image_resized, cv::Size(0,0), 0.50,0.50 );
        
        load_source_image.set_image(source_image_resized);
        source_image_resized = load_source_image.get_image();
        
        if (show_windows) {
            load_source_image.show();
        }
        
        
        Mat binary;
        cvtColor(source_image_resized, binary, CV_BGR2GRAY);
        threshold(binary, binary, 100, 255, CV_THRESH_BINARY);
        
        if (show_windows) {
            imshow("binary", binary);
        }
        
        
        Mat fg,bg;
        erode(binary, fg, Mat(),Point(-1,-1),3);
        if (show_windows) {
            imshow("fg", fg);
        }
        
        dilate(binary, bg, Mat(),Point(-1,-1),3);
        threshold(bg, bg, 1, 128, CV_THRESH_BINARY_INV);
        if (show_windows) {
            imshow("bg", bg);
        }
        
        
        Mat markers(binary.size(),CV_8U,Scalar(0));
        markers = fg + bg;
        if (show_windows) {
            imshow("markers", markers);
        }
        
        WatershedMarker wm;
        wm.set_markers(markers);
        
        Mat res,temp_res;
        res = wm.apply_watershed(source_image_resized);
        res.convertTo(res, CV_8U);
        if (show_windows) {
            imshow("res", res);
        }
        
//        imwrite("/Users/adarsh.kosuru/Desktop/pixelTrial/DerivedData/pixelTrial/Build/Products/Debug/res.jpg",res);
        
        
        Mat res_canny,res_copy,res_output;
        res.copyTo(res_copy);
        Canny(res_copy, res_canny, 100, 255);
        if (show_windows) {
            imshow("resCanny", res_canny);
        }
        
        
        
        
//        res.convertTo(res,CV_32S);
//        Rect br = boundingRect(res);
//        Mat biggest_rect(res,br);
        
        Mat image_contours;
        std::vector<std::vector<cv::Point>> main_contours;
        vector<cv::Vec4i> hierar;
        vector<int> indexes;
        Scalar color(0,255,0);
        findContours(res_canny, main_contours, squares_hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);
        res_output = Mat::zeros(res_canny.size(), CV_8UC3);
        
//        for (int draw_contours=0; draw_contours<main_contours.size(); draw_contours++) {
//            drawContours(res_output, main_contours, draw_contours, color,2,8,hierar,0,Point());
//        }
        
        cout << "Num of watershed contours - " << main_contours.size() << endl;
        
        drawContours(res_output, main_contours, -1, color,2,8,hierar,0,Point());
        
        if (show_windows) {
            imshow("contour", res_output);
        }

        
//        for (vector<vector<Point>>::iterator itr=main_contours.begin(); itr!=main_contours.end(); ++itr) {
//        }
        
        //main_contours = contours_image.set_source_image(res).find().get_contours();
        //squares_hierarchy = contours_image.get_hierarchy();
        
        for (int i=0; i<squares_hierarchy.size(); i++) {
            //        std::cout  << hierarchy[i] << std::endl;
            if (squares_hierarchy[i][3] == -1) {
                hierar.push_back(squares_hierarchy[i]);
                indexes.push_back(i);
            }
        }
        
        for (vector<int>::iterator itt = indexes.begin();itt!=indexes.end();++itt) {
            needed_contours.push_back(main_contours[*itt]);
        }
        
        int highest_area = 0,second_highest_area=0;
        Rect main_rect,second_highest_area_rect;
        vector<int> areas;
        vector<int> inserted_areas;
        vector<Rect> sorted_rectangles;
        for (vector<int>::iterator itt = indexes.begin();itt!=indexes.end();++itt) {
            Rect r = boundingRect(Mat(main_contours[*itt]));
//            cout << "area[" << *itt << "] - "<<r.area() <<endl;
            areas.push_back(r.area());
            all_rectangles.push_back(r);
            
            /*
            if (all_rectangles.size()==0) {
                all_rectangles.push_back(r);
            }
            else {
            for (vector<Rect>::iterator rect_iterator = all_rectangles.begin(); rect_iterator!=all_rectangles.end(); ++rect_iterator) {
                if (r.area() >  rect_iterator->area()) {
                    continue;
                }
                else{
                    all_rectangles.insert(all_rectangles.begin(), r);
                }
            }
            }
             */
            
            
//            if(*itt == 0){
//            main_rect = r;
//            }
            
            if (r.area() > highest_area) {
                second_highest_area=highest_area;
                highest_area = r.area();
                second_highest_area_rect = main_rect;
                main_rect = r;
            }
            
        }
        
        
        for (vector<Rect>::iterator rect_iterator = all_rectangles.begin(); rect_iterator!=all_rectangles.end(); ++rect_iterator) {
            
            if (inserted_areas.size()==0) {
                inserted_areas.push_back(rect_iterator->area());
                sorted_rectangles.push_back(*rect_iterator);
            }
            
            else{
                
                
                for (int sr=0; sr<sorted_rectangles.size(); sr++) {
                    if (rect_iterator->area() > sorted_rectangles[sr].area() && sr!=sorted_rectangles.size()-1) {
                        continue;
                    }
                    
                    else{
                        sorted_rectangles.insert(sorted_rectangles.begin() + sr, *rect_iterator);
                        break;
                    }
                }
                
            }
            
        }
        
        
        
        sort(areas.begin(), areas.end());
        sort(all_rectangles.begin(),all_rectangles.end(),mysortfunction);
        
        
        for (vector<Rect>::iterator sr=sorted_rectangles.begin(); sr!=sorted_rectangles.end(); ++sr) {
            cout << "sorted_rectangles_area[" << sr-sorted_rectangles.begin() <<"] - " <<sr->area() <<endl;
        }
        
        for (vector<Rect>::iterator ar=all_rectangles.begin(); ar!=all_rectangles.end(); ++ar) {
            cout << "all_rectangles_area[" << ar-all_rectangles.begin() <<"] - " <<ar->area() <<endl;
        }
        
        for (vector<int>::iterator areas_vector = areas.begin(); areas_vector!=areas.end(); ++areas_vector) {
            cout << "areas[" << areas_vector-areas.begin() <<"]- " << *areas_vector <<endl;
        }
        
        
        cout << "all_rectangles_size - " <<all_rectangles.size()<<endl;
        Mat cop = source_image_resized(second_highest_area_rect);
//        Mat cop = source_image_resized(all_rectangles[all_rectangles.size()-2]);
//        Mat cop = source_image_resized(main_rect);
//        resize(cop,cop,cv::Size(0,0),main_rect.width,main_rect.height);
        
        if (show_windows) {
            imshow("cop", cop);
        }
        
        bool houghp=true;
        
        
        
        if (!houghp) {
            vector<Vec2f> lines;
            Mat hough_image,hough_image_grayscale,hough_image_canny;
            cop.copyTo(hough_image);
            cvtColor(hough_image, hough_image_grayscale, CV_BGR2GRAY);
            Canny(hough_image_grayscale, hough_image_canny, 100, 255);
            
            HoughLines(hough_image_canny, lines, 1, CV_PI/180, 150);
            
            for( size_t i = 0; i < lines.size(); i++ )
            {
                float rho = lines[i][0], theta = lines[i][1];
                Point pt1, pt2;
                double a = cos(theta), b = sin(theta);
                double x0 = a*rho, y0 = b*rho;
                pt1.x = cvRound(x0 + 1000*(-b));
                pt1.y = cvRound(y0 + 1000*(a));
                pt2.x = cvRound(x0 - 1000*(-b));
                pt2.y = cvRound(y0 - 1000*(a));
                line( hough_image_grayscale, pt1, pt2, Scalar(255,0,0), 3, CV_AA);
            }
            
            imshow("houghResult", hough_image_grayscale);
            
            
            
            
            
        }
        else{
            vector<cv::Vec4i> lines;
            Mat hough_image,hough_image_grayscale,hough_image_canny;
//            source_image_resized.copyTo(hough_image);
            cop.copyTo(hough_image);
            cvtColor(hough_image, hough_image_grayscale, CV_BGR2GRAY);
            Canny(hough_image_grayscale, hough_image_canny, 100, 255);
//            HoughLinesP(hough_image_canny, lines, 1, CV_PI/180, 50);
            HoughLinesP(hough_image_canny, lines, 1, CV_PI/180, 50);
            
            Scalar hough_color = Scalar(255,0,0);
            
            
            
            //Expanded lines
            for (int el = 0; el<lines.size(); el++) {
                Vec4i v = lines[el];
                lines[el][0] = 0;
                lines[el][1] = ((float)v[1] - v[3]) / (v[0] - v[2]) * -v[0] + v[1];
                lines[el][2] = hough_image.cols;
                lines[el][3] = ((float)v[1] - v[3]) / (v[0] - v[2]) * (hough_image.cols - v[2]) + v[3];
            }
            
            vector<Vec4i>::const_iterator hough_iterator = lines.begin();
            while (hough_iterator!=lines.end()) {
                Point pt1((*hough_iterator)[0],(*hough_iterator)[1]);
                Point pt2((*hough_iterator)[2],(*hough_iterator)[3]);
                
                line(hough_image_grayscale, pt1, pt2, hough_color);
                
                ++hough_iterator;
            }
            
            imshow("houghResult",hough_image_grayscale);
            
            vector<Point2f> corners;
            cout << "corners size - " << corners.size() << endl;
            
            Size hough_size = hough_image.size();
            Point2f pt1(0,0);
//            Point2f pt2(200,0);
//            Point2f pt3(200,200);
//            Point2f pt4(0,200);
            Point2f pt2(hough_size.height,0);
            Point2f pt3(hough_size.height,hough_size.width);
            Point2f pt4(0,hough_size.width);
            corners.push_back(pt1);
            corners.push_back(pt2);
            corners.push_back(pt3);
            corners.push_back(pt4);
            
            cout << "quad size - " << hough_size.width << "," << hough_size.height <<endl;
//            Mat quad = Mat::zeros(200, 200, CV_8UC3);
            Mat quad = Mat::zeros(hough_size.height, hough_size.width, CV_8UC3);
            
            std::vector<cv::Point2f> quad_pts;
            quad_pts.push_back(cv::Point2f(0, 0));
            quad_pts.push_back(cv::Point2f(quad.cols, 0));
            quad_pts.push_back(cv::Point2f(quad.cols, quad.rows));
            quad_pts.push_back(cv::Point2f(0, quad.rows));
            
            Mat transpose_matrix = getPerspectiveTransform(corners, quad_pts);
            warpPerspective(hough_image, quad, transpose_matrix, quad.size());
            
            imshow("Quad", quad);
            
            
            
            
            
            /*
            for (int cr1=0; cr1<lines.size(); cr1++) {
                for (int cr2=cr1+1; cr2<lines.size(); cr2++) {
                    Point2f pt = find_intersection(lines[cr1],lines[cr2]);
                    
                    if (pt.x>=0 && pt.y>=0) {
                        corners.push_back(pt);
                    }
                }
            }
            
            cout << "corners size2 - " << corners.size() << endl;
            vector<Point2f> approx;
            Point2f center(0,0);
            approxPolyDP(Mat(corners), approx, arcLength(Mat(corners), true) * 0.02, true);
            
            if (approx.size() != 4) {
                cout << "Object doesnt have 4 corners" << endl;
            }
         
            else{
            for (int mass_center=0; mass_center<corners.size(); mass_center++) {
                center+=corners[mass_center];
            }
            center *= (1./corners.size());
            sort_corners(corners, center);
            
            Mat hough_image_destination = cop.clone();
            
            for (int l=0; l<lines.size(); l++) {
                Vec4i v = lines[l];
                line(hough_image_destination,Point(v[0],v[1]), Point(v[2],v[3]),CV_RGB(0,255,0));
            }
            
            Scalar color_pts(255,0,0);
            circle(hough_image_destination, corners[0], 3, color_pts,2);
            circle(hough_image_destination, corners[1], 3, color_pts,2);
            circle(hough_image_destination, corners[2], 3, color_pts,2);
            circle(hough_image_destination, corners[3], 3, color_pts,2);
            
            circle(hough_image_destination,center,3,color_pts,2);
            Size s = hough_image_destination.size();
            cout << "size - " << s.width << ","<<s.height<<endl;
            imshow("pts",hough_image_destination);
            
            Mat quad = Mat::zeros(300, 220, CV_8UC3);
            
            std::vector<cv::Point2f> quad_pts;
            quad_pts.push_back(cv::Point2f(0, 0));
            quad_pts.push_back(cv::Point2f(quad.cols, 0));
            quad_pts.push_back(cv::Point2f(quad.cols, quad.rows));
            quad_pts.push_back(cv::Point2f(0, quad.rows));
            
            }*/
        }
        
    }
    
    
     bool Components::mysortfunction(Rect r1, Rect r2){
        
         
         return r1.area()<r2.area();
    }
    
    Point2f Components::find_intersection(Vec4i a,Vec4i b){
        
        int x1 = a[0], y1 = a[1], x2 = a[2], y2 = a[3], x3 = b[0], y3 = b[1], x4 = b[2], y4 = b[3];

        if (float d = ((float)(x1 - x2) * (y3 - y4)) - ((y1 - y2) * (x3 - x4)))
        {
            cv::Point2f pt;
            pt.x = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / d;
            pt.y = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / d;
            return pt;
        }
        else
            return cv::Point2f(-1, -1);
        
    }
    
    void Components::sort_corners(vector<Point2f>& corners, Point2f center){
        vector<Point2f> top, bot;
        
        for (int i=0; i<corners.size(); i++) {
            if (corners[i].y < center.y) {
                top.push_back(corners[i]);
            }
            else{
                bot.push_back(corners[i]);
            }
        }
        
        cv::Point2f tl = top[0].x > top[1].x ? top[1] : top[0];
        cv::Point2f tr = top[0].x > top[1].x ? top[0] : top[1];
        cv::Point2f bl = bot[0].x > bot[1].x ? bot[1] : bot[0];
        cv::Point2f br = bot[0].x > bot[1].x ? bot[0] : bot[1];
        
        corners.clear();
        corners.push_back(tl);
        corners.push_back(tr);
        corners.push_back(br);
        corners.push_back(bl);
    }
    
    void Components::rotate_image(Mat& input, double angle,Mat& rotated_image){
        int length = std::max(input.cols, input.rows);
        Point2f pt(length/2.,length/2.);
        Mat rt = getRotationMatrix2D(pt, angle, 1.0);
        warpAffine(input, rotated_image, rt, Size(length,length));
    }
}