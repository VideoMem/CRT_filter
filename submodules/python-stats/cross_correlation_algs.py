from scipy.io import wavfile as wav
from scipy.signal import butter, filtfilt, resample
import pandas as pd
import numpy as np
#matplotlib inline
import matplotlib.pyplot as plt
#import seaborn as sns
import scipy.stats as stats

def log3_2( data ):
    return np.log(data) / np.log ( 1.5 )

def log_compress ( data ):
    data_max = np.max(np.abs(data))
    data_norm = data / (data_max * 1.2)
    return log3_2( ( data_norm + 2 ) / 2)

def loop_compress( data, level ):
    loop = data
    for i in range(level):
        loop = log_compress( loop )
    return loop

def downsample(data, srate, newrate):
    newshape = round(data.size * newrate / srate)
    if srate != newrate:
        return resample(data, newshape)
    else:
        return data

def rms(data):
    audiodata = data.astype(np.float64)
    rms = np.sqrt(audiodata**2)
    return rms.reshape(data.shape)

def mono_mix(data):
    audiodata = data.astype(np.float64)
    return audiodata.sum(axis=1) / 0xFFFF

def butter_bandpass(lowcut, highcut, fs, order=5):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a

def butter_bandpass_filter(data, lowcut, highcut, fs, order=5):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = filtfilt(b, a, data)
    return y

def load_wave( filename ):
    rate, samples = wav.read( filename )

    print(f"Sample Rate: {rate}")
    channels = samples.shape[1]
    print(f"Number of channels = {channels}")
    length = samples.shape[0] / rate
    print(f"length = {length}s")

    if channels > 1:
        mono = mono_mix(samples)
    else:
        mono = samples

    return rate, mono

def hash_rms( data ):
    rmsv = rms(data)
    rmslp = butter_bandpass_filter( rmsv, lowcut=lowest, highcut=mid_lo, fs=rate, order=2)
    return log_compress(rmslp)

def pandas_pearson_r( df ):
    overall_pearson_r = df.corr().iloc[0,1]
    print(f"Pandas computed Pearson r: {overall_pearson_r}")
    return overall_pearson_r

def pearson( df ):
    overall_pearson_r = pandas_pearson_r(df)
    r, p = stats.pearsonr( df.dropna()['original'], df.dropna()['transform'] )
    print(f"Scipy computed Pearson r: {r} and p-value: {p}")

    f,ax=plt.subplots(figsize=(14,3))
    df.rolling(window=30,center=True).median().plot(ax=ax)
    ax.set(xlabel='Sample',ylabel='Correlation',title=f"Overall Pearson r = {np.round(overall_pearson_r,2)}")
    #plt.show()

def threshold_sync( data, level ):
    i = 0
    for sample in data:
        if np.abs( sample ) > level: return i
        i+=1

def crosscorr(datax, datay, lag=0, wrap=False):
    """ Lag-N cross correlation.
    Shifted data filled with NaNs

    Parameters
    ----------
    lag : int, default 0
    datax, datay : pandas.Series objects of equal length

    Returns
    ----------
    crosscorr : float
    """

    if wrap:
        shiftedy = datay.shift(lag)
        shiftedy.iloc[:lag] = datay.iloc[-lag:].values
        return datax.corr(shiftedy)
    else:
        return datax.corr(datay.shift(lag))

def slice_part( data, start, end, margin ):
    if start > margin:
        if data.size - end > margin:
            return data[start - margin: end + margin ]
        else:
            return data[start - margin: end ]
    else:
        if data.size - end > margin:
            return data[start: end + margin ]
        else:
            return data[start: end ]

def crosscorr_offset( d1, d2, downrate, seconds ):
    rs = [ crosscorr( d1 , d2, lag ) for lag in range( -int(seconds*downrate), int(seconds*downrate+1) ) ]
    return rs, np.ceil( len(rs) / 2 ) - np.argmax(rs)

def crosscorr_plot( rs, offset ):
    f , ax = plt.subplots( figsize=( 14, 3 ) )
    ax.plot(rs)
    ax.axvline(np.ceil(len(rs)/2),color='k',linestyle='--',label='Center')
    ax.axvline(np.argmax(rs),color='r',linestyle='--',label='Peak synchrony')
    ax.set(title=f'Offset = {offset} frames\nS1 leads <> S2 leads',ylim=[.1,.31],xlim=[0,301], xlabel='Offset',ylabel='Pearson r')
    ax.set_xticks([0, 50, 100, 151, 201, 251, 301])
    ax.set_xticklabels([-150, -100, -50, 0, 50, 100, 150]);
    plt.legend()

def to_dataframe( d0, d1, org_ini, org_end, cpy_ini, cpy_end, rate, downrate ):
    original = downsample( slice_part( d0, org_ini, org_end, 1024 ), rate, downrate)
    transform = downsample( slice_part( d1, cpy_ini, cpy_end, 1024 ), rate, downrate)
    p_original = original[:transform.size]
    df = pd.DataFrame({ 'original':p_original, 'transform':transform })
    return df

def pearson_slicer( df, start, step ):
    pearson_r = 1
    while np.abs(pearson_r) > 0.05:
        slice = df[ start: start + step ]
        pearson_r = pandas_pearson_r( slice )
        start += step
    return int( start - step ), pearson_r

def pearson_filter( df ):
    bits = int(np.log(df.size) / np.log( 2 ))
    print( f'bits {bits}')
    newstart = 0
    for i in range(1, bits):
        step = int( df.size / 2**i )
        if newstart - step > 0:
            start = newstart - step
        else:
            start = newstart
        print( f'start {start}, step {step}' )
        newstart, pearson_r = pearson_slicer( df, start, step )
        if np.abs(pearson_r) < 0.05: break
    return int( newstart )

def gain_range( original, transform, start, end, divisor ):
    error_f = []
    for gain in range( start, end ):
        error_f.append( np.mean( rms( original - transform * gain / divisor ) ) )
    return error_f

def gain_min( error, start, end ):
    min_error = np.min( error )
    id = 0
    for gain in range( start, end ):
        if error[id] == min_error:
            return gain
        id+=1
    return None

def autogain( original, transform ):
    error_f = gain_range( original, transform, 2, 18, 10 )
    tens = gain_min( error_f, 2, 18 )
    print( f'10: min error at gain:{ tens }')
    error_f = gain_range( original, transform, (tens - 1) * 10, (tens + 1) * 10, 100 )
    hundreds = gain_min( error_f, (tens - 1) * 10, (tens + 1) * 10 )
    print( f'100: min error at gain:{ hundreds }')
    error_f = gain_range( original, transform, (hundreds - 1) * 10, (hundreds + 1) * 10, 1000 )
    thousands = gain_min( error_f, (hundreds - 1) * 10, (hundreds + 1) * 10 )
    print( f'1000: min error at gain:{ thousands }')
    return thousands / 1000

rate, mono = load_wave( 'sample00.wav' )
none, copy = load_wave( 'correlated.wav' )

lowest= 5
mid_lo = 80
mid_hi = 3000
highest = rate /2 -1


lowband = butter_bandpass_filter( mono, lowcut=lowest, highcut=mid_lo, fs=rate, order=2)
lowbcpy = butter_bandpass_filter( copy, lowcut=lowest, highcut=mid_lo, fs=rate, order=2)
#midband = butter_bandpass_filter( mono, lowcut=mid_lo, highcut=mid_hi, fs=rate, order=3)
#higband = butter_bandpass_filter( mono, lowcut=mid_hi, highcut=highest, fs=rate, order=2)

rmslog = hash_rms( lowband )
rmscpy = hash_rms( lowbcpy )
reversed_rmslog = rmslog[::-1]
reversed_rmscpy = rmslog[::-1]
wav.write( "rmslo.wav", rate, rmslog )
wav.write( "rmscp.wav", rate, rmscpy )


#thresold sync focus
th_org_ini = threshold_sync( rmslog, 0.01 )
th_cpy_ini = threshold_sync( rmscpy, 0.01 )
th_org_end = rmslog.size - threshold_sync( reversed_rmslog, 0.01 )
th_cpy_end = rmscpy.size - threshold_sync( reversed_rmscpy, 0.01 )
if th_org_end - th_org_ini < th_cpy_end - th_cpy_ini:
    copy_len = th_org_end - th_org_ini
    th_cpy_end = th_cpy_ini + copy_len

print( f'original ini: {th_org_ini} ~ {th_org_end} end' )
print( f'transform ini: {th_cpy_ini} ~ {th_cpy_end} end' )

downrate = round( mid_lo * 2.2 )

df = to_dataframe( rmslog, rmscpy, th_org_ini, th_org_end, th_cpy_ini, th_cpy_end, rate, downrate )
pearson( df )
seconds = 1
rs, offset = crosscorr_offset( df['original'], df['transform'], downrate, seconds )
crosscorr_plot( rs, offset )

print( f'offset: {offset}' )

##offset error of threshold sync done
# sync correction

th_org_ini -= int( offset * rate / downrate )
dmx = to_dataframe( rmslog, rmscpy, th_org_ini, th_org_end, th_cpy_ini, th_cpy_end, rate, downrate )
pearson( dmx )
drs, doffset = crosscorr_offset( dmx['original'], dmx['transform'], downrate, seconds )
crosscorr_plot( drs, doffset )
print( f'offset after: {doffset}' )

newending = pearson_filter( dmx )
print( f'original ending: {dmx.size}, new ending: {newending}' )


total_len = int(newending * rate / downrate) - th_cpy_ini
th_cpy_end = th_cpy_ini + total_len
th_org_end = th_org_ini + total_len
newsynced = copy[th_cpy_ini: th_cpy_end ]
orgsynced = mono[th_org_ini: th_org_end ]


dfs = to_dataframe( orgsynced, newsynced, 0, seconds * rate, 0, seconds * rate, rate, rate )
rss, hi_offset = crosscorr_offset( dfs['original'], dfs['transform'], rate, seconds / 2 )
#crosscorr_plot( rs, hi_offset )
print( f'hi offset: {hi_offset}' )

th_org_ini -= int( hi_offset ) -1
th_org_end -= int( hi_offset ) -1

orgsynced = mono[th_org_ini: th_org_end ]

c_gain = autogain( orgsynced[:rate*seconds], newsynced[:rate*seconds] )

print( f'min error at gain: {c_gain }')

print( f'len {total_len} newsync: {newsynced.size} orgsync: {orgsynced.size}' )
synced  = np.transpose ( np.asarray ( (orgsynced, newsynced * c_gain) ) )
print( f'synced shape: {synced.shape}' )
wav.write( "resynced.wav", rate, synced )

error = orgsynced - newsynced * c_gain
wav.write( "error.wav", rate, error )

#plt.show()


