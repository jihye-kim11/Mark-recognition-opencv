#include "opencv2/opencv.hpp"
#include <iostream>

using namespace std;
using namespace cv;

//mat���� �̹��� �޾ƿ��� �Լ� ¥��


int main()
{
	VideoCapture cap(0); // open the default camera

	Mat input_image = imread("land.jpg", IMREAD_COLOR);//�̹��� color�� �ҷ�����,���� ����
	Mat input_gray_image;
	cvtColor(input_image, input_gray_image, CV_BGR2GRAY);//���ó��
	Mat binary_image;

	threshold(input_gray_image, binary_image, 125, 255, THRESH_BINARY_INV | THRESH_OTSU);//����ȭ

	
	Mat contour_image = binary_image.clone();
	vector<vector<Point> > contours;//������ ����
	vector<Vec4i> hierarchy;

	findContours(contour_image, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);//������ ����


	Mat drawing = Mat::zeros(contour_image.size(), CV_8UC3);
	for (int i = 0; i < contours.size(); i++)
	{
    drawContours(drawing, contours, i, Scalar(0, 255, 0), 1, 8, hierarchy);
	}
	imshow("������", drawing);

	//contour�� �ٻ�ȭ�Ѵ�.
	vector<vector<Point2f> > marker;
	vector<Point2f> approx;

	for (size_t i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.05, true);
		//approxPolyDP(�־��� �ٰ���,��´ٰ���,�������� ��� �Ÿ�,true�� ����� false�� ���� �)
		if (
			approx.size() == 4 && //�簢���� 4���� vertex�� ������. 
			//���̰� 5�� 4�� 2���� �簢�� ã�Ƽ� ���� �簢�� �ν� or 10�� 9�� �簢�� ã�Ƽ� �ν�
			fabs(contourArea(Mat(approx))) > 120000 && //������ ����ũ�� �̻��̾�� �Ѵ�.
			fabs(contourArea(Mat(approx))) < 2100000 && //������ ����ũ�� ���Ͽ��� �Ѵ�.
			//���� ���� ���� ũ�� �ٲ��ٵ� ��� ����?????
			isContourConvex(Mat(approx)) //convex���� �˻��Ѵ�.
			)
		{

		drawContours(input_image, contours, i, Scalar(0, 255, 0), 1, LINE_AA);
		imshow("�簢��",input_image);

			vector<cv::Point2f> points;
			for (int j = 0; j<4; j++)
				points.push_back(cv::Point2f(approx[j].x, approx[j].y));

			//�ݽð� �������� ����,�� ��ġ �ٻ�ȭ
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

	int marker_image_side_length = 500; //��Ŀ ������ �׵θ� ���� ������ ũ��� 10*10
									   //���� �ܰ迡�� �̹����� ���ڷ� ������ �� ���ϳ��� �ȼ��ʺ� 10���� �Ѵٸ�
									   //��Ŀ �̹����� �Ѻ� ���̴� 500
	square_points.push_back(cv::Point2f(0, 0));
	square_points.push_back(cv::Point2f(marker_image_side_length - 1, 0));
	square_points.push_back(cv::Point2f(marker_image_side_length - 1, marker_image_side_length - 1));
	square_points.push_back(cv::Point2f(0, marker_image_side_length - 1));
	//������ �簢�� �� ������

	Mat marker_image;
	for (int i = 0; i < marker.size(); i++)
	{
		vector<Point2f> m = marker[i];

		//Mat input_gray_image2 = input_gray_image.clone();
		//Mat markerSubImage = input_gray_image2(cv::boundingRect(m));


		//��Ŀ�� �簢�����·� �ٲ� perspective transformation matrix�� ���Ѵ�.
		Mat PerspectiveTransformMatrix = getPerspectiveTransform(m, square_points);

		//perspective transformation�� �����Ѵ�. 
		warpPerspective(input_gray_image, marker_image, PerspectiveTransformMatrix, Size(marker_image_side_length, marker_image_side_length));

		//otsu ������� ����ȭ�� �����Ѵ�. 
		threshold(marker_image, marker_image, 125, 255, THRESH_BINARY | THRESH_OTSU);



		//��Ŀ�� ũ��� 8, ������ �µθ��� ������ ũ��� 10
		//��Ŀ �̹��� �׵θ��� �˻��Ͽ� ���� ���������� Ȯ���Ѵ�. 
		int cellSize = marker_image.rows / 10;
		int white_cell_count = 0;
		for (int y = 0; y<10; y++)
		{
			int inc = 9; // ù��° ���� ������ ���� �˻��ϱ� ���� ��

			if (y == 0 || y == 9) inc = 1; //ù��° �ٰ� ���������� ��� ���� �˻��Ѵ�. 


			for (int x = 0; x<10; x += inc)
			{
				int cellX = x * cellSize;
				int cellY = y * cellSize;
				cv::Mat cell = marker_image(Rect(cellX, cellY, cellSize, cellSize));

				int total_cell_count = countNonZero(cell);

				if (total_cell_count >(cellSize*cellSize) / 2)
					white_cell_count++; //�µθ��� ��������� �ִٸ�, ������ �ȼ��� �����̻� ����̸� ����������� ���� 

			}
		}

		//������ �µθ��� �ѷ��׿� �ִ� �͸� �����Ѵ�.
		if (white_cell_count == 0) {
			detectedMarkers.push_back(m);
			Mat img = marker_image.clone();
			detectedMarkersImage.push_back(img);
		}
	}

	imshow("��Ŀ�� �ν�", marker_image);

	for (int i = 0; i < detectedMarkers.size(); i++)
	{
		Mat marker_image = detectedMarkersImage[i];

		//���� 8x8�� �ִ� ������ ��Ʈ�� �����ϱ� ���� ����
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
			//cout << bitMatrix << endl << endl;//1,0���� ��Ÿ�� ��Ʈ���� ���
			unsigned char data[] = { 1, 1, 1, 1, 1, 1, 1, 1,1, 1, 1, 0, 0, 1, 1, 1,1, 1, 1, 0, 0, 1, 1, 1,1, 0, 0, 1, 1, 0, 0, 1,1, 0, 0, 1, 1, 0, 0, 1,1, 1, 1, 0, 0, 1, 1, 1,1, 1, 1, 0, 0, 1, 1, 1,1, 1, 1, 1, 1, 1, 1, 1 };
			Mat takeoff = Mat(8, 8, CV_8UC1,data);

			for (int y = 0; y < 8; y++)
			{
				for (int x = 0; x < 8; x++)
				{
					if ((int)bitMatrix.at<uchar>(y, x) == (int)takeoff.at<uchar>(y, x))
					{//cout << y << x << "�Ϸ�" << endl;
					}
     				else
					{
						goto LAND;
					}
				}
				
				//cout << y << "�Ϸ�" << endl;
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
					{	//cout << y << x << "�Ϸ�" << endl;
					}
					else
					{
						goto ELSE;
					}
				}
				//cout << y << "�Ϸ�" << endl;
				if (y == 7)
				{
					cout << "LAND MARKER" << endl;
					return 2;
					goto END;
				}
			}

		ELSE:
			cout << "takeoff land ��� �ƴϴ�" << endl;
	}
 
END:

	//imshow("rge->gray",img);//���ó��
	//waitKey();

	return 0;
}