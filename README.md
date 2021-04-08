# Mark-recognition-opencv

-.opencv를 이용하여 드론이 마커를 인식하여 이륙,착륙 명령하는 코드 제작하기.
1) 받아온 이미지를 input_image->흑백처리(cvtColor)->이진화(threshold)과정을 통해 원본파일 단순화 시킨다.
2)모든 cotour 찾기->주어진 다각형 중 드론마커의 가장 큰 겉면이 되는 사각형을 찾는다.
3)input_image에서 드론마커 부분만 잘라 추출한다.
4)사진에서 인식한 사각형의 10*10 cell을 흑백 기준으로 하여 흑일경우 1 백일경우 0인 mat 행렬로 저장
5)takeoff 마커와 land마커를 0*10 cell을 흑백 기준으로 하여 흑일경우 1 백일경우 0인 mat 행렬로 저장
6)탐색대상인 사각형의 행렬을 takeoff 행렬과 land 행렬과 각각 비교후 take인지 land인지 확인.(if문,goto문 사용)
7)드론으로 실행되는지 확인
