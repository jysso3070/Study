import smtplib
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email import encoders
import os

def SendEmail(EmailAdress, text, SearchPoint):
    me = 'jysso3070@naver.com'
    you = EmailAdress
    contents = "검색기준주소: " + SearchPoint + "\n\n가까운대피소 검색 결과: \n" + text

    naver_server = smtplib.SMTP_SSL('smtp.naver.com', 465)
    naver_server.login('jysso3070', 'dbswntn3030')

    msg = MIMEBase('multipart', 'mixed')

    cont = MIMEText(contents)
    cont['Subject'] = '스크립트언어'
    cont['From'] = me
    cont['To'] = you

    msg.attach(cont)

    path = 'nearSearch.html'
    part = MIMEBase("application", "octet-stream")
    part.set_payload(open(path, 'rb').read())

    encoders.encode_base64(part)
    part.add_header('Content-Disposition', 'attachment; filename="%s"'% os.path.basename(path))

    msg.attach(part)
    naver_server.sendmail(me, you, msg.as_string())
    naver_server.quit()

# def SendEmailVer2(EmailAdress, text, SearchPoint):
#