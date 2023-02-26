import dataLoad
import dataClass
import sendEmail
from haversine import haversine
from tkinter import *
from tkinter import font
import tkinter.messagebox
import folium
import webbrowser
import tkinter.ttk
import kakaoApi

bookMarkData = list()
sheltDataList = []

Data = dataLoad.LoadData()
for data in Data.find_all("record"):
    name = data.sheltitle.string
    type = data.type.string
    normal = data.normal.string
    adressRoad = data.adress.string
    adress2 = data.adress2.string
    latitude = data.latitude.string
    longitude = data.longitude.string
    capacity = data.people.string
    tel = data.tel.string
    shelt = dataClass.shelterData(name,type, normal,adressRoad, adress2,latitude,longitude, capacity, tel)
    sheltDataList.append(shelt)


window = Tk()
window.title("민방위 대피소 어플리케이션")
window.geometry("600x600")
notebook=tkinter.ttk.Notebook(window, width=550, height=450)
notebook.pack()
notebook.place(x=10, y=90)
frame1 = tkinter.Frame(window)
notebook.add(frame1, text="주소검색")
frame2 = tkinter.Frame(window)
notebook.add(frame2, text="가까운대피소")
frame3 = tkinter.Frame(window)
notebook.add(frame3, text="지역별 분류")
frame4 = tkinter.Frame(window)
notebook.add(frame4, text="즐겨찾기")


def topTitle():
    tempFont = font.Font(window, size=30, weight="bold", family="Consolas")
    # title = Label(window, font=tempFont, text="[전국 민방위 대피소 App]")
    # title.place(x=5, y=10)
    image = PhotoImage(file="a.gif")
    label =Label(window, image=image)
    label.image = image
    label.place(x=10,y=10)

def initInputLabel():
    global InputLabel
    TempFont = font.Font(frame1, size=15, weight='bold', family='Consolas')
    InputLabel = Entry(frame1, font=TempFont, width=26, borderwidth=12, relief='ridge')
    InputLabel.place(x=0, y=10)

def initSearchButton():
    TempFont = font.Font(frame1, size=12, weight='bold', family = 'Consolas')
    SearchButton = Button(frame1, font = TempFont, borderwidth = 10, text="검색", command=searchButtonProcess)
    SearchButton.place(x=320, y=10)

def searchButtonProcess():
    global searchTemp
    # initRenderText()
    listbox.delete(0,20000)
    searchTemp = str(InputLabel.get())
    for data in sheltDataList:
        if searchTemp in str(data.adress2) or searchTemp in str(data.adressRoad):
            try:
                text = data.shelterName+"  ["+data.adress2+"]"

            except:
                print("error")
                if data.adress2 == None:
                    text = data.shelterName+" ["+data.adressRoad+"]"
            listbox.insert(END, text)

def initRenderText():
    global listbox
    RenderTextScrollbarY = Scrollbar(frame1)
    RenderTextScrollbarX = Scrollbar(frame1, orient=HORIZONTAL)
    RenderTextScrollbarY.pack(side=RIGHT, fill=Y)
    RenderTextScrollbarX.pack(side=BOTTOM, fill=X)
    listbox = Listbox(frame1,selectmode='browse', height=15, width=60, relief='ridge',  borderwidth=12,
                      xscrollcommand = RenderTextScrollbarX.set, yscrollcommand = RenderTextScrollbarY.set)
    listbox.place(x=0,y=100)
    RenderTextScrollbarY.config(command=listbox.yview)
    RenderTextScrollbarX.config(command=listbox.xview)

def initInfoButton():
    TempFont = font.Font(frame1, size=11, weight='bold', family='Consolas')
    SearchButton = Button(frame1, font=TempFont, width=7, height=2, borderwidth=5, text="상세정보", command=InfoButtonAction)
    SearchButton.pack()
    SearchButton.place(x=450, y=100)

def InfoButtonAction():
    searchIndex = str(listbox.curselection())
    try:
        searchIndex = int(searchIndex[1:-2]) + 1
    except:
        pass
    print(searchIndex)
    tempIndex=0
    for data in sheltDataList:
        if searchTemp in str(data.adress2) or searchTemp in str(data.adressRoad):
            tempIndex += 1
            if searchIndex == tempIndex:
                try:
                    tkinter.messagebox.showinfo("대피소 상세정보", "이름: "+data.shelterName+"\n주소: "+data.adress2+
                                                "\n평시활용:"+data.normal+"\n전화번호: "+data.telNum+"\n수용인원: "+data.capacity+"명")
                except:
                    print("errorInfo")
                    tempAdress = ""
                    tempNormal = ""
                    tempTelNum = ""
                    tempCapa = ""
                    if data.adress2 == None:
                        tempAdress = "\n주소: " + data.adressRoad
                    elif data.adressRoad == None:
                        tempAdress = "\n주소: [정보 없음]"
                    else:
                        tempAdress = "\n주소: "+data.adress2
                    if data.normal == None:
                        tempNormal = "\n평시활용: [정보 없음]"
                    else:
                        tempNormal = "\n평시활용: "+data.normal
                    if data.telNum == None:
                        tempTelNum = "\n전화번호: [정보 없음]"
                    else:
                        tempTelNum = "\n전화번호: " + data.telNum
                    if data.capacity == None:
                        tempCapa = "\n수용인원: [정보없음]"
                    else:
                        tempCapa = "\n수용인원: " +data.capacity
                    tkinter.messagebox.showinfo("대피소 상세정보", "이름: " + data.shelterName + tempAdress +
                                                tempNormal + tempTelNum + tempCapa)





def initMapButton():
    TempFont = font.Font(frame1, size=11, weight='bold', family='Consolas')
    SearchButton = Button(frame1, font=TempFont, width=7, height=2, borderwidth=5, text="지도출력", command=MapButtonAction)
    SearchButton.pack()
    SearchButton.place(x=450, y=160)

def MapButtonAction():
    searchIndex = str(listbox.curselection())
    try:
        searchIndex = int(searchIndex[1:-2]) + 1
    except:
        pass
    tempIndex = 0
    for data in sheltDataList:
        if searchTemp in str(data.adress2):
            tempIndex += 1
            try:
                if searchIndex == tempIndex:
                    map_osm = folium.Map(location=[float(data.latitude), float(data.longitude)], zoom_start=16)
                    folium.Marker([data.latitude, data.longitude], popup=data.shelterName,
                                  icon = folium.Icon(color = 'red', icon = 'info-sign')).add_to(map_osm)
                    # filepath = data.shelterName+".html"
                    filepath = "shelterMap.html"
                    map_osm.save(filepath)
                    webbrowser.open_new_tab(filepath)
            except:
                pass
#2번째탭함수------------------------------------------------------------------------------------------
def initInputLabelNote2():
    global InputLabelNote2
    TempFont = font.Font(frame2, size=15, weight='bold', family='Consolas')
    InputLabelNote2 = Entry(frame2, font=TempFont, width=24, borderwidth=12, relief='ridge')
    InputLabelNote2.place(x=0, y=10)

def initSearchButtonNote2():
    TempFont = font.Font(frame2, size=12, weight='bold', family = 'Consolas')
    SearchButton = Button(frame2, font = TempFont, borderwidth = 10, text="가까운대피소찾기", command=searchNearShelProcess)
    SearchButton.place(x=295, y=10)

def initCheckButtonNote2():
    global CheckVarNote2
    CheckVarNote2 = IntVar()
    checkAdress = Checkbutton(frame2, text="주소검색", variable=CheckVarNote2)
    checkAdress.place(x=460, y=10)

def initRenderTextNote2():
    global textNote2
    global myAdressLabel
    textNote2 = Text(frame2, height=10, width=60, relief='ridge', borderwidth=12)
    textNote2.place(x=0,y=100)
    myAdressLabel = Label(frame2, text=" ")
    myAdressLabel.place(x=0, y=60)

def searchNearShelProcess():
    textNote2.delete(1.0, END)
    global shortDistanceInfo
    global myLocation
    global nearShelterText
    global myAdress
    searchNearTemp= str(InputLabelNote2.get())
    resultData = kakaoApi.kakaoApiKeywordSearch(searchNearTemp)
    myLatitude = float(resultData["documents"][0]["y"])
    myLongitude = float(resultData["documents"][0]["x"])
    myAdress = str(resultData["documents"][0]["address_name"])
    myLocation = (myLatitude, myLongitude)
    shortDistance = 200
    shortDistanceInfo = sheltDataList[0]
    if myLongitude < 0:
        myAdressLabel.configure(text="검색불가능")
    else:
        myAdressLabel.configure(text="검색기준주소: " + myAdress)
    for data in sheltDataList:
        tempLocation = (float(data.latitude), float(data.longitude))
        distance = haversine(myLocation, tempLocation)
        if shortDistance > distance:
            shortDistance = distance
            shortDistanceInfo = data
    print(myLatitude)
    print(myLongitude)
    print(shortDistance)
    try:
        nearShelterText = "명칭: " + shortDistanceInfo.shelterName + "\n주소: " + shortDistanceInfo.adress2 + \
               "\n전화번호: " + shortDistanceInfo.telNum + "\n수용인원: " + shortDistanceInfo.capacity
    except:
        if shortDistanceInfo.adress2 == None:
            nearShelterText = "명칭: " + shortDistanceInfo.shelterName + "\n주소: " + shortDistanceInfo.adressRoad + \
                   "\n전화번호: " + shortDistanceInfo.telNum + "\n수용인원: " + shortDistanceInfo.capacity
        elif shortDistanceInfo.telNum == None:
            nearShelterText = "명칭: " + shortDistanceInfo.shelterName + "\n주소: " + shortDistanceInfo.adressRoad + \
                   "\n전화번호: [정보없음]" + "\n수용인원: " + shortDistanceInfo.capacity
        elif shortDistanceInfo.capacity == None:
            nearShelterText = "명칭: " + shortDistanceInfo.shelterName + "\n주소: " + shortDistanceInfo.adressRoad + \
                   "\n전화번호: " + shortDistanceInfo.telNum + "\n수용인원: [정보없음]"
    textNote2.insert(END, nearShelterText)
    if shortDistance >= 1:
        shortDistance = round(shortDistance, 2)
        textNote2.insert(END, "\n거리: "+str(shortDistance)+"km")
    else:
        shortDistance *= 1000
        shortDistance = round(shortDistance)
        textNote2.insert(END, "\n거리: " + str(shortDistance) + "m")
    if myLongitude < 0:
        textNote2.delete(1.0, END)

def initMapButtonNote2():
    TempFont = font.Font(frame2, size=11, weight='bold', family='Consolas')
    SearchButton = Button(frame2, font=TempFont, width=7, height=2, borderwidth=5, text="지도출력", command=MapButtonActionNote2)
    SearchButton.pack()
    SearchButton.place(x=450, y=100)

def MapButtonActionNote2():
    map_osm = folium.Map(location=[float(myLocation[0]), float(myLocation[1])], zoom_start=15)
    folium.Marker([shortDistanceInfo.latitude, shortDistanceInfo.longitude], popup=shortDistanceInfo.shelterName,
                  icon=folium.Icon(color='red', icon='info-sign')).add_to(map_osm)
    folium.Marker([myLocation[0], myLocation[1]], popup="검색 기준 위치",
                  icon=folium.Icon(color='blue', icon='info-sign')).add_to(map_osm)
    # filepath = shortDistanceInfo.shelterName + "near.html"
    filepath = "nearSearch.html"
    map_osm.save(filepath)
    webbrowser.open_new_tab(filepath)

def initInsertBookMarkButtonNote2():
    TempFont = font.Font(frame2, size=11, weight='bold', family='Consolas')
    InsertBookMark = Button(frame2, font=TempFont, width=7, height=2, borderwidth=5, text="즐겨찾기",
                          command=InsertBookMarkProgressNote2)
    InsertBookMark.pack()
    InsertBookMark.place(x=450, y=160)

def InsertBookMarkProgressNote2():
    tempData = shortDistanceInfo
    for data in bookMarkData:
        if tempData.shelterName == data.shelterName and tempData.adressRoad == data.adressRoad:
            tkinter.messagebox.showwarning("중복데이터", "이미 즐겨찾기 되어있습니다.")
            print("이미즐겨찾기 되어있음")
            return
    bookMarkData.append(tempData)
    tkinter.messagebox.showinfo("줄겨찾기 성공", "즐겨찾기 목록에 추가 되었습니다.")
    renderBookMarkNote4()

def initEmailEntryNote2():
    global InputEmailAdress
    TempFont = font.Font(frame2, size=15, weight='bold', family='Consolas')
    InputEmailAdress = Entry(frame2, font=TempFont, width=24, borderwidth=12, relief='ridge')
    InputEmailAdress.place(x=0, y=300)

def initSendEmailButtonNote2():
    TempFont = font.Font(frame2, size=11, weight='bold', family='Consolas')
    SendButton = Button(frame2, font=TempFont, borderwidth=10, text="이메일전송", command=SendEmailButtonProgress)
    SendButton.pack()
    SendButton.place(x=295, y=300)

def SendEmailButtonProgress():
    emailAdress = str(InputEmailAdress.get())
    sendEmialText = nearShelterText
    searchPoint = myAdress
    sendEmail.SendEmail(emailAdress, sendEmialText, searchPoint)

def AllMap():
    map_osm = folium.Map(location=[37.3402849, 126.7335076], zoom_start=15)
    i=0
    for data in sheltDataList:
        folium.Marker([data.latitude, data.longitude], popup=str(i),
                      icon=folium.Icon(color='red', icon='info-sign')).add_to(map_osm)
        i+=1
    filepath = "AllSearch.html"
    map_osm.save(filepath)

#3번째탭함수------------------------------------------------------------------------------------------
def initCheckRegionNote3():
    global RadioVarNote3
    RadioVarNote3 = StringVar()
    Seoul = Radiobutton(frame3, text="서울특별시", variable = RadioVarNote3, value="서울특별시")
    Seoul.place(x=0,y=10)
    Incheon = Radiobutton(frame3, text="인천광역시", variable=RadioVarNote3, value="인천광역시")
    Incheon.place(x=0, y=30)
    Daejeon = Radiobutton(frame3, text="대전광역시", variable=RadioVarNote3, value="대전광역시")
    Daejeon.place(x=0, y=50)
    Gwangju = Radiobutton(frame3, text="광주광역시", variable=RadioVarNote3, value="광주광역시")
    Gwangju.place(x=0, y=70)
    Ulsan = Radiobutton(frame3, text="울산광역시", variable=RadioVarNote3, value="울산광역시")
    Ulsan.place(x=0, y=90)
    Daegu = Radiobutton(frame3, text="대구광역시", variable=RadioVarNote3, value="대구광역시")
    Daegu.place(x=0, y=110)
    Busan = Radiobutton(frame3, text="부산광역시", variable=RadioVarNote3, value="부산광역시")
    Busan.place(x=0, y=130)
    Gyeonggi = Radiobutton(frame3, text="경기도", variable=RadioVarNote3, value="경기도")
    Gyeonggi.place(x=0, y=150)
    Gangwon = Radiobutton(frame3, text="강원도", variable=RadioVarNote3, value="강원도")
    Gangwon.place(x=0, y=170)
    Chungbuk = Radiobutton(frame3, text="충청북도", variable=RadioVarNote3, value="충청북도")
    Chungbuk.place(x=0, y=190)
    Chungnam = Radiobutton(frame3, text="충청남도", variable=RadioVarNote3, value="충청남도")
    Chungnam.place(x=0, y=210)
    Jeonbuk = Radiobutton(frame3, text="전라북도", variable=RadioVarNote3, value="전라북도")
    Jeonbuk.place(x=0, y=230)
    Jeonnam = Radiobutton(frame3, text="전라남도", variable=RadioVarNote3, value="전라남도")
    Jeonnam.place(x=0, y=250)
    Gyeongbuk = Radiobutton(frame3, text="경상북도", variable=RadioVarNote3, value="경상북도")
    Gyeongbuk.place(x=0, y=270)
    Gyeongnam = Radiobutton(frame3, text="경상남도", variable=RadioVarNote3, value="경상남도")
    Gyeongnam.place(x=0, y=290)
    Jeju = Radiobutton(frame3, text="제주도", variable=RadioVarNote3, value="제주특별자치도")
    Jeju.place(x=0, y=310)

def initSearchButtonNote3():
    TempFont = font.Font(frame3, size=10, weight='bold', family='Consolas')
    SearchButton = Button(frame3, font=TempFont, borderwidth=5, text="검색", command=searchRegionProcess)
    SearchButton.place(x=90, y=10)

def initRenderResultNote3():
    global listNote3
    RenderTextScrollbarY = Scrollbar(frame3)
    RenderTextScrollbarX = Scrollbar(frame3, orient=HORIZONTAL)
    RenderTextScrollbarY.pack(side=RIGHT, fill=Y)
    RenderTextScrollbarX.pack(side=BOTTOM, fill=X)
    listNote3 = Listbox(frame3, selectmode='browse', height=20, width=55, relief='ridge', borderwidth=5,
                     xscrollcommand=RenderTextScrollbarX.set, yscrollcommand=RenderTextScrollbarY.set)
    listNote3.place(x=90, y=70)
    RenderTextScrollbarY.config(command=listNote3.yview)
    RenderTextScrollbarX.config(command=listNote3.xview)

def resultCountLabelNote3():
    global resultCountLabel
    resultCountLabel = Label(frame3, text="")
    resultCountLabel.place(x=90, y=50)

def searchRegionProcess():
    searchRegion = str(RadioVarNote3.get())
    print(searchRegion)
    listNote3.delete(0, END)
    resultCount = 0
    for data in sheltDataList:
        if searchRegion in str(data.adress2):
            try:
                text = data.shelterName+"  ["+data.adress2+"]"
                listNote3.insert(END, text)
                resultCount+=1
            except:
                print("error")
    resultCountLabel.configure(text="검색결과: " + str(resultCount) + "곳")

def initMapButtonNote3():
    TempFont = font.Font(frame3, size=10, weight='bold', family='Consolas')
    SearchButton = Button(frame3, font=TempFont, borderwidth=5, text="지도출력", command=MapButtonProcessNote3)
    SearchButton.place(x=140, y=10)

def MapButtonProcessNote3():
    searchRegion = str(RadioVarNote3.get())
    for data in sheltDataList:
        if searchRegion in str(data.adress2):
            map_osm = folium.Map(location=[data.latitude, data.longitude], zoom_start=10)
            break
    for data in sheltDataList:
        if searchRegion in str(data.adress2):
            folium.Marker([data.latitude, data.longitude], popup=str(data.shelterName),
                          icon=folium.Icon(color='red', icon='info-sign')).add_to(map_osm)
    filepath = "regionSearch.html"
    map_osm.save(filepath)
    webbrowser.open_new_tab(filepath)


#-----------------------------------------탭4 함수--------------------------------------------
def initRenderBookMarkNote4():
    global BookMarkListbox
    RenderTextScrollbarY = Scrollbar(frame4)
    RenderTextScrollbarX = Scrollbar(frame4, orient=HORIZONTAL)
    RenderTextScrollbarY.pack(side=RIGHT, fill=Y)
    RenderTextScrollbarX.pack(side=BOTTOM, fill=X)
    BookMarkListbox = Listbox(frame4,selectmode='browse', height=15, width=60, relief='ridge',  borderwidth=12,
                      xscrollcommand = RenderTextScrollbarX.set, yscrollcommand = RenderTextScrollbarY.set)
    BookMarkListbox.place(x=0,y=10)
    RenderTextScrollbarY.config(command=BookMarkListbox.yview)
    RenderTextScrollbarX.config(command=BookMarkListbox.xview)

def initInfoButtonNote4():
    TempFont = font.Font(frame4, size=11, weight='bold', family='Consolas')
    InfoButton = Button(frame4, font=TempFont, width=7, height=2, borderwidth=5, text="상세정보",
                          command=InfoButtonProgressNote4)
    InfoButton.pack()
    InfoButton.place(x=450, y=10)

def InfoButtonProgressNote4():
    bookMarkIndex = str(BookMarkListbox.curselection())
    try:
        bookMarkIndex = int(bookMarkIndex[1:-2]) + 1
    except:
        print("북마크인덱스가져오기 오류")
    print(bookMarkIndex)
    tempIndex = 0
    for data in bookMarkData:
        tempIndex +=1
        if tempIndex == bookMarkIndex:
            try:
                tkinter.messagebox.showinfo("대피소 상세정보", "이름: " + data.shelterName + "\n주소: " + data.adress2 +
                                            "\n평시활용:" + data.normal + "\n전화번호: " + data.telNum + "\n수용인원: " + data.capacity + "명")
            except:
                print("errorInfo")
                tempAdress = ""
                tempNormal = ""
                tempTelNum = ""
                tempCapa = ""
                if data.adress2 == None:
                    tempAdress = "\n주소: " + data.adressRoad
                elif data.adressRoad == None:
                    tempAdress = "\n주소: [정보 없음]"
                else:
                    tempAdress = "\n주소: " + data.adress2
                if data.normal == None:
                    tempNormal = "\n평시활용: [정보 없음]"
                else:
                    tempNormal = "\n평시활용: " + data.normal
                if data.telNum == None:
                    tempTelNum = "\n전화번호: [정보 없음]"
                else:
                    tempTelNum = "\n전화번호: " + data.telNum
                if data.capacity == None:
                    tempCapa = "\n수용인원: [정보없음]"
                else:
                    tempCapa = "\n수용인원: " + data.capacity
                tkinter.messagebox.showinfo("대피소 상세정보", "이름: " + data.shelterName + tempAdress +
                                            tempNormal + tempTelNum + tempCapa)

def initBookMarkMapButtonNote4():
    TempFont = font.Font(frame4, size=11, weight='bold', family='Consolas')
    MapBookMark = Button(frame4, font=TempFont, width=7, height=2, borderwidth=5, text="지도출력",
                            command=BookMarkMapButtonProgress)
    MapBookMark.pack()
    MapBookMark.place(x=450, y=70)

def BookMarkMapButtonProgress():
    bookMarkIndex = str(BookMarkListbox.curselection())
    try:
        bookMarkIndex = int(bookMarkIndex[1:-2]) + 1
    except:
        print("북마크인덱스가져오기 오류")
    data = bookMarkData[bookMarkIndex-1]
    map_osm = folium.Map(location=[float(data.latitude), float(data.longitude)], zoom_start=16)
    folium.Marker([data.latitude, data.longitude], popup=data.shelterName,
                  icon=folium.Icon(color='red', icon='info-sign')).add_to(map_osm)
    filepath = "BookMarkMap.html"
    map_osm.save(filepath)
    webbrowser.open_new_tab(filepath)


def initDeleteBookMarkButtonNote4():
    TempFont = font.Font(frame4, size=11, weight='bold', family='Consolas')
    deleteBookMark = Button(frame4, font=TempFont, width=7, height=2, borderwidth=5, text="삭제",
                        command=deleteBookMarkButtonProgress)
    deleteBookMark.pack()
    deleteBookMark.place(x=450, y=130)

def deleteBookMarkButtonProgress():
    bookMarkIndex = str(BookMarkListbox.curselection())
    try:
        bookMarkIndex = int(bookMarkIndex[1:-2]) + 1
    except:
        print("북마크인덱스가져오기 오류")
    del bookMarkData[bookMarkIndex-1]
    renderBookMarkNote4()


def renderBookMarkNote4():
    BookMarkListbox.delete(0, END)
    if len(bookMarkData) == 0:
        return
    else:
        for data in bookMarkData:
            BookMarkListbox.insert(END ,data.shelterName + " ["+data.adressRoad+"]")
    saveBookMark()

def saveBookMark():
    print("리스트수"+str(len(bookMarkData)))
    f = open("bookmarklist.txt", "w")
    for data in bookMarkData:
        tempname = data.shelterName
        tempadress = data.adressRoad
        print(tempname)
        print(tempadress)
        f.write(tempname+"\n")
        f.write(tempadress+"\n")
    f.close()

def roadBookMark():
    fo = open("bookmarklist.txt", "r")
    n = 0
    while True:
        tempN = str(fo.readline())
        tempA = str(fo.readline())
        tempN = tempN[:-1]
        tempA = tempA[:-1]
        n += 1
        if not tempN:
            break
        print(tempN)
        print(tempA)
        for data in sheltDataList:
            if data.shelterName == tempN and data.adressRoad ==tempA:
                bookMarkData.append(data)
    print(n)
    fo.close()
    renderBookMarkNote4()




topTitle()
#----------------------------1탭
initInputLabel()
initSearchButton()
initRenderText()
initInfoButton()
initMapButton()
#---------------------------2탭
initInputLabelNote2()
initSearchButtonNote2()
# initCheckButtonNote2()
initRenderTextNote2()
initMapButtonNote2()
initInsertBookMarkButtonNote2()
initEmailEntryNote2()
initSendEmailButtonNote2()
#----------------------------3탭
initCheckRegionNote3()
resultCountLabelNote3()
initSearchButtonNote3()
initRenderResultNote3()
initMapButtonNote3()
#------------------------------4탭
initRenderBookMarkNote4()
initBookMarkMapButtonNote4()
initDeleteBookMarkButtonNote4()
initInfoButtonNote4()
roadBookMark()

window.mainloop()