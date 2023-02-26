
class shelterData:
    def __init__(self, shelterName, type, normal,roadAdress, adress2, latitude, longitude, capacity, tel):
        self.shelterName = shelterName
        self.type = type
        self.normal = normal
        self.adressRoad = roadAdress
        self.adress2 = adress2
        self.latitude = float(latitude)
        self.longitude = float(longitude)
        self.capacity = capacity
        self.telNum = tel

    def showData(self):
        print("시설명: ", self.shelterName)
        print("지번주소: ", self.adress2)
        print("도로명주소: ", self.adressRoad)
        print("위도: ", self.latitude)
        print("경도: ", self.longitude)
        print("수용인원: ", self.capacity)
        print("전화번호: ", self.telNum)










    def getShelterName(self):
        return self.shelterName

    def getNormal(self):
        return self.normal

    def getRoadAdress(self):
        return self.adressRoad
