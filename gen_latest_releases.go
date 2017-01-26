package main;

import (
    "fmt"
    "os"
    "strings"
    "net/http"
    "io/ioutil"
    "bufio"
)

func Exit(value int)  {
    os.Exit(value);
}

func Check(err error) {
    if err != nil {
        fmt.Printf("Error: %s\n", err);
        Exit(1);
    }
}

func UrlDataGet(url string) string {
    req, err := http.Get(url);
    Check(err);
    defer req.Body.Close();

    if req.StatusCode != http.StatusOK {
        return "";
    }

    bytes, err := ioutil.ReadAll(req.Body);

    return string(bytes);
}


func ParseForLatestDebian(arch string) string {
    base_url := "http://cdimage.debian.org/debian-cd/current/multi-arch/iso-cd";

    data := UrlDataGet(base_url)

    tag_start := "<a href=\"";
    tag_full := tag_start + "debian-"
    idx := strings.Index(data, tag_full);

    str := data[idx + len(tag_start) :len(data)];
    end := strings.Index(str, "\""); 
    filename := str[0:end]

    return fmt.Sprintf("%s/%s", base_url, filename);
}

func newestURL(data string, major_start int, major_end int) string {
    var found string = "";
    for major := major_start; major < major_end; major++ {
        for minor := 0; minor < 10; minor++ {
            tmp := fmt.Sprintf("%d.%d", major, minor)
            if strings.Contains(data, tmp) {
                found = tmp;
            }
        }
    }
    return found;
}

func ParseForLatestFreeBSD(arch string) string {

    const FREEBSD_MAJOR_START = 11
    const FREEBSD_MAJOR_END = 20
    base_url := "http://ftp.freebsd.org/pub/FreeBSD/releases/ISO-IMAGES";

    data := UrlDataGet(base_url);

    var version string = newestURL(data, FREEBSD_MAJOR_START, FREEBSD_MAJOR_END);

    if version != "" {
        return fmt.Sprintf("%s/%s/FreeBSD-%s-RELEASE-%s-memstick.img", base_url, version, version, arch);
    }

    return version;
}

func ParseForLatestOpenBSD(arch string) string {
    const OPENBSD_MAJOR_START = 6;
    const OPENBSD_MAJOR_END = 10;
    base_url := "http://mirror.ox.ac.uk/pub/OpenBSD/";
 
    data := UrlDataGet(base_url); 

    var version string  = newestURL(data, OPENBSD_MAJOR_START, OPENBSD_MAJOR_END);

    // hack for changing 6.0 to 60 for filename on server
    img_ver := strings.Replace(version, ".", "", -1);
    
    if version != "" {
        return fmt.Sprintf("%s%s/%s/install%s.fs", base_url, version, arch, img_ver);
    }

    return "";
}

func LatestReleaseURL(os string, arch string) string {
    var url string = "";

    switch (os) {
        case "OpenBSD":
        url = ParseForLatestOpenBSD(arch);
        break;

        case "FreeBSD":
        url = ParseForLatestFreeBSD(arch);
        break;

        case "Debian":
        url = ParseForLatestDebian(arch);
        break;
    }

    if len(url) == 0 {
        fmt.Printf("Error: LatestReleaseURL %s %s\n", os, arch);
        Exit(1);
    }

    return url;
}

func distroGetAll() map[string]string {

    distros := make(map[string]string);

    distros["OpenBSD (amd64)"] = LatestReleaseURL("OpenBSD", "amd64");
    distros["OpenBSD (i386)"]  = LatestReleaseURL("OpenBSD", "i386");  
    
    distros["FreeBSD (i386)"]  = LatestReleaseURL("FreeBSD", "i386");
    distros["FreeBSD (amd64)"] = LatestReleaseURL("FreeBSD", "amd64");
    
    distros["Debian GNU/Linux (i386/amd64)"] = LatestReleaseURL("Debian", "amd64");

    return distros;
}

func main() {

    distros := distroGetAll();

    f, err := os.Create("list.txt");
    Check(err);
    defer f.Close();

    w := bufio.NewWriter(f);

    for name, url := range distros {
        fmt.Fprintf(w, "%s=%s\n", name, url);
        fmt.Printf("Name: %s and URL %s\n", name, url);
    }

    w.Flush();

    Exit(0);
}