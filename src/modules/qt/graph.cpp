/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "graph.h"

/*
 * Apply the "bgcolor" and "angle" parameters to the QPainter
 */
void setup_graph_painter( QPainter& p, QRectF& r, mlt_properties filter_properties )
{
	mlt_color bg_color = mlt_properties_get_color( filter_properties, "bgcolor" );
	double angle = mlt_properties_get_double( filter_properties, "angle" );

	p.setRenderHint(QPainter::Antialiasing);

	// Fill background
	if ( bg_color.r || bg_color.g || bg_color.g || bg_color.a ) {
		QColor qbgcolor( bg_color.r, bg_color.g, bg_color.b, bg_color.a );
		p.fillRect( 0, 0, p.device()->width(), p.device()->height(), qbgcolor );
	}

	// Apply rotation
	if ( angle ) {
		p.translate( r.x() + r.width() / 2, r.y() + r.height() / 2 );
		p.rotate( angle );
		p.translate( -(r.x() + r.width() / 2), -(r.y() + r.height() / 2) );
	}
}

/*
 * Apply the "thickness", "gorient" and "color.x" parameters to the QPainter
 */
void setup_graph_pen( QPainter& p, QRectF& r, mlt_properties filter_properties )
{
	int thickness = mlt_properties_get_int( filter_properties, "thickness" );
	QString gorient = mlt_properties_get( filter_properties, "gorient" );
	QVector<QColor> colors;
	bool color_found = true;

	QPen pen;
	pen.setWidth( qAbs(thickness) );

	// Find user specified colors for the gradient
	while( color_found ) {
		QString prop_name = QString("color.") + QString::number(colors.size() + 1);
		if( mlt_properties_get(filter_properties, prop_name.toUtf8().constData() ) ) {
			mlt_color mcolor = mlt_properties_get_color( filter_properties, prop_name.toUtf8().constData() );
			colors.append( QColor( mcolor.r, mcolor.g, mcolor.b, mcolor.a ) );
		} else {
			color_found = false;
		}
	}

	if( !colors.size() ) {
		// No color specified. Just use white.
		pen.setBrush(Qt::white);
	} else if ( colors.size() == 1 ) {
		// Only use one color
		pen.setBrush(colors[0]);
	} else {
		QLinearGradient gradient;
		if( gorient.startsWith("h", Qt::CaseInsensitive) ) {
			gradient.setStart ( r.x(), r.y() );
			gradient.setFinalStop ( r.x() + r.width(), r.y() );
		} else { // Vertical
			gradient.setStart ( r.x(), r.y() );
			gradient.setFinalStop ( r.x(), r.y() + r.height() );
		}

		qreal step = 1.0 / ( colors.size() - 1 );
		for( int i = 0; i < colors.size(); i++ )
		{
			gradient.setColorAt( (qreal)i * step, colors[i] );
		}
		pen.setBrush(gradient);
	}

	p.setPen(pen);
}
