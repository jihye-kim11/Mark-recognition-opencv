#include "opencv2/opencv.hpp"
#include <iostream>

using namespace std;
using namespace cv;

//mat으로 이미지 받아오게 함수 짜기


int main()
{
	VideoCapture cap(0); // open the default camera

	Mat input_image = imread("land.jpg", IMREAD_COLOR);//이미지 color로 불러오기,투명 무시
	Mat input_gray_image;
	cvtColor(input_image, input_gray_image, CV_BGR2GRAY);//흑백처리
	Mat binary_image;

	threshold(input_gray_image, binary_image, 125, 255, THRESH_BINARY_INV | THRESH_OTSU);//이진화

	
	Mat contour_image = binary_image.clone();
	vector<vector<Point> > contours;//윤곽선 저장
	vector<Vec4i> hierarchy;

	findContours(contour_image, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);//윤곽선 추적


	Mat drawing = Mat::zeros(contour_image.size(), CV_8UC3);
	for (int i = 0; i < contours.size(); i++)
	{
    drawContours(drawing, contours, i, Scalar(0, 255, 0), 1, 8, hierarchy);
	}
	imshow("윤곽선", drawing);

	//contour를 근사화한다.
	vector<vector<Point2f> > marker;
	vector<Point2f> approx;

	for (size_t i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.05, true);
		//approxPolyDP(주어진 다각형,출력다각형,직선과의 허용 거리,true면 닫힌곡선 false면 열린 곡선)
		if (
			approx.size() == 4 && //사각형은 4개의 vertex를 가진다. 
			//넙이가 5대 4인 2개읭 사각형 찾아서 안쪽 사각형 인식 or 10대 9인 사각형 찾아서 인식
			fabs(contourArea(Mat(approx))) > 120000 && //면적이 일정크기 이상이어야 한다.
			fabs(contourArea(Mat(approx))) < 2100000 && //면적이 일정크기 이하여야 한다.
			//영상에 따라 면적 크기 바뀔텐데 어떻게 설정?????
			isContourConvex(Mat(approx)) //convex인지 검사한다.
			)
		{

		drawContours(input_image, contours, i, Scalar(0, 255, 0), 1, LINE_AA);
		imshow("사각형",input_image);

			vector<cv::Point2f> points;
			for (int j = 0; j<4; j++)
				points.push_back(cv::Point2f(approx[j].x, approx[j].y));

			//반시계 방향으로 정렬,점 위치 근사화
			cv::Point v1 = points[1] - points[0];
			cv::Point v2 = points[2] - points[0];

			double o = (v1.x * v2.y) - (v1.y * v2.x);
			if (o < 0.0)
				swap(points[1], points[3]);

			marker.push_back(points);

		}
	}


	vector<vector<Point2f> > detectedMarkers;
	vector<Mat> detectedMarkersImage;
	vector<Point2f> square_points;//2d

	int marker_image_side_length = 500; //마커 검은색 테두리 영역 포함한 크기는 10*10
									   //이후 단계에서 이미지를 격자로 분할할 시 셀하나의 픽셀너비를 10으로 한다면
									   //마커 이미지의 한변 길이는 500
	square_points.push_back(cv::Point2f(0, 0));
	square_points.push_back(cv::Point2f(marker_image_side_length - 1, 0));
	square_points.push_back(cv::Point2f(marker_image_side_length - 1, marker_image_side_length - 1));
	square_points.push_back(cv::Point2f(0, marker_image_side_length - 1));
	//추출할 사각형 각 꼭짓점

	Mat marker_image;
	for (int i = 0; i < marker.size(); i++)
	{
		vector<Point2f> m = marker[i];

		//Mat input_gray_image2 = input_gray_image.clone();
		//Mat markerSubImage = input_gray_image2(cv::boundingRect(m));


		//마커를 사각형형태로 바꿀 perspective transformation matrix를 구한다.
		Mat PerspectiveTransformMatrix = getPerspectiveTransform(m, square_points);

		//perspective transformation을 적용한다. 
		warpPerspective(input_gray_image, marker_image, PerspectiveTransformMatrix, Size(marker_image_side_length, marker_image_side_length));

		//otsu 방법으로 이진화를 적용한다. 
		threshold(marker_image, marker_image, 125, 255, THRESH_BINARY | THRESH_OTSU);



		//마커의 크기는 8, 검은색 태두리를 포함한 크기는 10
		//마커 이미지 테두리만 검사하여 전부 검은색인지 확인한다. 
		int cellSize = marker_image.rows / 10;
		int white_cell_count = 0;
		for (int y = 0; y<10; y++)
		{
			int inc = 9; // 첫번째 열과 마지막 열만 검사하기 위한 값

			if (y == 0 || y == 9) inc = 1; //첫번째 줄과 마지막줄은 모든 열을 검사한다. 


			for (int x = 0; x<10; x += inc)
			{
				int cellX = x * cellSize;
				int cellY = y * cellSize;
				cv::Mat cell = marker_image(Rect(cellX, cellY, cellSize, cellSize));

				int total_cell_count = countNonZero(cell);

				if (total_cell_count >(cellSize*cellSize) / 2)
					white_cell_count++; //태두리에 흰색영역이 있다면, 셀내의 픽셀이 절반이상 흰색이면 흰색영역으로 본다 

			}
		}

		//검은색 태두리로 둘러쌓여 있는 것만 저장한다.
		if (white_cell_count == 0) {
			detectedMarkers.push_back(m);
			Mat img = marker_image.clone();
			detectedMarkersImage.push_back(img);
		}
	}

	imshow("마커만 인식", marker_image);

	for (int i = 0; i < detectedMarkers.size(); i++)
	{
		Mat marker_image = detectedMarkersImage[i];

		//내부 8x8에 있는 정보를 비트로 저장하기 위한 변수
		Mat bitMatrix(8, 8, CV_8UC1, Scalar::all(0));

		int cellSize = marker_image.rows / 10;

		for (int y = 0; y < 8; y++)
		{
			for (int x = 0; x < 8; x++)
			{
				int cellX = (x + 1)*cellSize;
				int cellY = (y + 1)*cellSize;
				Mat cell = marker_image(cv::Rect(cellX, cellY, cellSize, cellSize));

				int total_cell_count = countNonZero(cell);


				if (total_cell_count >(cellSize*cellSize) / 2)
					bitMatrix.at<uchar>(y, x) = 1;

			}
		}
			//cout << bitMatrix << endl << endl;//1,0으로 나타낸 매트리스 출력
			unsigned char data[] = { 1, 1, 1, 1, 1, 1, 1, 1,1, 1, 1, 0, 0, 1, 1, 1,1, 1, 1, 0, 0, 1, 1, 1,1, 0, 0, 1, 1, 0, 0, 1,1, 0, 0, 1, 1, 0, 0, 1,1, 1, 1, 0, 0, 1, 1, 1,1, 1, 1, 0, 0, 1, 1, 1,1, 1, 1, 1, 1, 1, 1, 1 };
			Mat takeoff = Mat(8, 8, CV_8UC1,data);

			for (int y = 0; y < 8; y++)
			{
				for (int x = 0; x < 8; x++)
				{
					if ((int)bitMatrix.at<uchar>(y, x) == (int)takeoff.at<uchar>(y, x))
					{//cout << y << x << "완료" << endl;
					}
     				else
					{
						goto LAND;
					}
				}
				
				//cout << y << "완료" << endl;
					if (y == 7) 
					{
						cout << "TAKE OFF MARKER" << endl;
						return 1;
						goto END;
					}
			}
			
		LAND:
			unsigned char data_[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
				Mat land = Mat(8, 8, CV_8UC1, data_);

			for (int y = 0; y < 8; y++)
			{
				for (int x = 0; x < 8; x++)
				{
					if ((int)bitMatrix.at<uchar>(y, x) == (int)land.at<uchar>(y, x))
					{	//cout << y << x << "완료" << endl;
					}
					else
					{
						goto ELSE;
					}
				}
				//cout << y << "완료" << endl;
				if (y == 7)
				{
					cout << "LAND MARKER" << endl;
					return 2;
					goto END;
				}
			}

		ELSE:
			cout << "takeoff land 모두 아니다" << endl;
	}
 
END:

	//imshow("rge->gray",img);//흑백처리
	//waitKey();

	return 0;
}